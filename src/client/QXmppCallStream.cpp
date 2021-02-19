/*
 * Copyright (C) 2020 The QXmpp developers
 *
 * Author:
 *  Niels Ole Salscheider
 *
 * Source:
 *  https://github.com/qxmpp-project/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "QXmppCallStream.h"

#include "QXmppCallStream_p.h"
#include "QXmppCall_p.h"
#include "QXmppStun.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

#include <cstring>

#include <gst/gst.h>

QXmppCallStreamPrivate::QXmppCallStreamPrivate(QXmppCallStream *parent, GstElement *pipeline_,
                                               GstElement *rtpbin_, QString media_, QString creator_,
                                               QString name_, int id_)
    : QObject(parent),
      q(parent),
      pipeline(pipeline_),
      rtpbin(rtpbin_),
      sendPad(nullptr),
      receivePad(nullptr),
      encoderBin(nullptr),
      decoderBin(nullptr),
      sendPadCB(nullptr),
      receivePadCB(nullptr),
      media(media_),
      creator(creator_),
      name(name_),
      id(id_)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    localSsrc = QRandomGenerator::global()->generate();
#else
    localSsrc = qrand();
#endif

    iceReceiveBin = gst_bin_new(QStringLiteral("receive_%1").arg(id).toLatin1().data());
    iceSendBin = gst_bin_new(QStringLiteral("send_%1").arg(id).toLatin1().data());
    gst_bin_add_many(GST_BIN(pipeline), iceReceiveBin, iceSendBin, nullptr);

    internalRtpPad = gst_ghost_pad_new_no_target(nullptr, GST_PAD_SINK);
    internalRtcpPad = gst_ghost_pad_new_no_target(nullptr, GST_PAD_SINK);
    if (!gst_element_add_pad(iceSendBin, internalRtpPad) ||
        !gst_element_add_pad(iceSendBin, internalRtcpPad)) {
        qFatal("Failed to add pads to send bin");
    }

    connection = new QXmppIceConnection(this);
    connection->addComponent(RTP_COMPONENT);
    connection->addComponent(RTCP_COMPONENT);
    apprtpsink = gst_element_factory_make("appsink", nullptr);
    apprtcpsink = gst_element_factory_make("appsink", nullptr);
    if (!apprtpsink || !apprtcpsink) {
        qFatal("Failed to create appsinks");
    }

    g_signal_connect_swapped(apprtpsink, "new-sample",
                             G_CALLBACK(+[](QXmppCallStreamPrivate *p, GstElement *appsink) -> GstFlowReturn {
                                 return p->sendDatagram(appsink, RTP_COMPONENT);
                             }),
                             this);
    g_signal_connect_swapped(apprtcpsink, "new-sample",
                             G_CALLBACK(+[](QXmppCallStreamPrivate *p, GstElement *appsink) -> GstFlowReturn {
                                 return p->sendDatagram(appsink, RTCP_COMPONENT);
                             }),
                             this);

    apprtpsrc = gst_element_factory_make("appsrc", nullptr);
    apprtcpsrc = gst_element_factory_make("appsrc", nullptr);
    if (!apprtpsrc || !apprtcpsrc) {
        qFatal("Failed to create appsrcs");
    }

    // TODO check these parameters
    g_object_set(apprtpsink, "emit-signals", true, "async", false, "max-buffers", 1, "drop", true, nullptr);
    g_object_set(apprtcpsink, "emit-signals", true, "async", false, nullptr);
    g_object_set(apprtpsrc, "is-live", true, "max-latency", 5000000, nullptr);
    g_object_set(apprtcpsrc, "is-live", true, nullptr);

    connect(connection->component(RTP_COMPONENT), &QXmppIceComponent::datagramReceived,
            [&](const QByteArray &datagram) { datagramReceived(datagram, apprtpsrc); });
    connect(connection->component(RTCP_COMPONENT), &QXmppIceComponent::datagramReceived,
            [&](const QByteArray &datagram) { datagramReceived(datagram, apprtcpsrc); });

    if (!gst_bin_add(GST_BIN(iceReceiveBin), apprtpsrc) ||
        !gst_bin_add(GST_BIN(iceReceiveBin), apprtcpsrc)) {
        qFatal("Failed to add appsrcs to receive bin");
    }

    if (!gst_element_link_pads(apprtpsrc, "src", rtpbin, QStringLiteral("recv_rtp_sink_%1").arg(id).toLatin1().data()) ||
        !gst_element_link_pads(apprtcpsrc, "src", rtpbin, QStringLiteral("recv_rtcp_sink_%1").arg(id).toLatin1().data())) {
        qFatal("Failed to link receive pads");
    }

    // We need frequent RTCP reports for the bandwidth controller
    GstElement *rtpSession;
    g_signal_emit_by_name(rtpbin, "get-session", static_cast<uint>(id), &rtpSession);
    g_object_set(rtpSession, "rtcp-min-interval", 100000000, nullptr);

    gst_element_sync_state_with_parent(iceReceiveBin);
    gst_element_sync_state_with_parent(iceSendBin);
}

QXmppCallStreamPrivate::~QXmppCallStreamPrivate()
{
    connection->close();

    // Remove elements from pipeline
    if ((encoderBin && !gst_bin_remove(GST_BIN(pipeline), encoderBin)) ||
        (decoderBin && !gst_bin_remove(GST_BIN(pipeline), decoderBin)) ||
        !gst_bin_remove(GST_BIN(pipeline), iceSendBin) ||
        !gst_bin_remove(GST_BIN(pipeline), iceReceiveBin)) {
        qFatal("Failed to remove bins from pipeline");
    }
}

GstFlowReturn QXmppCallStreamPrivate::sendDatagram(GstElement *appsink, int component)
{
    GstSample *sample;
    g_signal_emit_by_name(appsink, "pull-sample", &sample);
    if (!sample) {
        qFatal("Could not get sample");
        return GST_FLOW_ERROR;
    }

    GstMapInfo mapInfo;
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        qFatal("Could not get buffer");
        return GST_FLOW_ERROR;
    }
    if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
        qFatal("Could not map buffer");
        return GST_FLOW_ERROR;
    }
    QByteArray datagram;
    datagram.resize(mapInfo.size);
    std::memcpy(datagram.data(), mapInfo.data, mapInfo.size);
    gst_buffer_unmap(buffer, &mapInfo);
    gst_sample_unref(sample);

    if (connection->component(component)->isConnected() &&
        connection->component(component)->sendDatagram(datagram) != datagram.size()) {
        return GST_FLOW_ERROR;
    }
    return GST_FLOW_OK;
}

void QXmppCallStreamPrivate::datagramReceived(const QByteArray &datagram, GstElement *appsrc)
{
    GstBuffer *buffer = gst_buffer_new_and_alloc(datagram.size());
    GstMapInfo mapInfo;
    if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE)) {
        qFatal("Could not map buffer");
        return;
    }
    std::memcpy(mapInfo.data, datagram.data(), mapInfo.size);
    gst_buffer_unmap(buffer, &mapInfo);
    GstFlowReturn ret;
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
    gst_buffer_unref(buffer);
}

void QXmppCallStreamPrivate::addEncoder(QXmppCallPrivate::GstCodec &codec)
{
    // Remove old encoder and payloader if they exist
    if (encoderBin) {
        if (!gst_bin_remove(GST_BIN(pipeline), encoderBin)) {
            qFatal("Failed to remove existing encoder bin");
        }
    }
    encoderBin = gst_bin_new(QStringLiteral("encoder_%1").arg(id).toLatin1().data());
    if (!gst_bin_add(GST_BIN(pipeline), encoderBin)) {
        qFatal("Failed to add encoder bin to wrapper");
        return;
    }

    sendPad = gst_ghost_pad_new_no_target(nullptr, GST_PAD_SINK);
    gst_element_add_pad(encoderBin, sendPad);

    // Create new elements
    GstElement *queue = gst_element_factory_make("queue", nullptr);
    if (!queue) {
        qFatal("Failed to create queue");
        return;
    }

    GstElement *pay = gst_element_factory_make(codec.gstPay.toLatin1().data(), nullptr);
    if (!pay) {
        qFatal("Failed to create payloader");
        return;
    }
    g_object_set(pay, "pt", codec.pt, "ssrc", localSsrc, nullptr);

    GstElement *encoder = gst_element_factory_make(codec.gstEnc.toLatin1().data(), nullptr);
    if (!encoder) {
        qFatal("Failed to create encoder");
        return;
    }
    for (auto &encProp : codec.encProps) {
        g_object_set(encoder, encProp.name.toLatin1().data(), encProp.value, nullptr);
    }

    gst_bin_add_many(GST_BIN(encoderBin), queue, encoder, pay, nullptr);

    if (!gst_element_link_pads(pay, "src", rtpbin, QStringLiteral("send_rtp_sink_%1").arg(id).toLatin1().data()) ||
        !gst_element_link_many(queue, encoder, pay, nullptr)) {
        qFatal("Could not link all encoder pads");
        return;
    }

    if (!gst_ghost_pad_set_target(GST_GHOST_PAD(sendPad), gst_element_get_static_pad(queue, "sink"))) {
        qFatal("Failed to set send pad");
        return;
    }

    if (sendPadCB) {
        sendPadCB(sendPad);
    }

    gst_element_sync_state_with_parent(encoderBin);

    addRtcpSender(gst_element_get_request_pad(rtpbin, QStringLiteral("send_rtcp_src_%1").arg(id).toLatin1().data()));
}

void QXmppCallStreamPrivate::addDecoder(GstPad *pad, QXmppCallPrivate::GstCodec &codec)
{
    // Remove old decoder and depayloader if they exist
    if (decoderBin) {
        if (!gst_bin_remove(GST_BIN(pipeline), decoderBin)) {
            qFatal("Failed to remove existing decoder bin");
        }
    }
    decoderBin = gst_bin_new(QStringLiteral("decoder_%1").arg(id).toLatin1().data());
    if (!gst_bin_add(GST_BIN(pipeline), decoderBin)) {
        qFatal("Failed to add decoder bin to wrapper");
        return;
    }

    receivePad = gst_ghost_pad_new_no_target(nullptr, GST_PAD_SRC);
    internalReceivePad = gst_ghost_pad_new_no_target(nullptr, GST_PAD_SINK);
    gst_element_add_pad(decoderBin, receivePad);
    gst_element_add_pad(decoderBin, internalReceivePad);

    // Create new elements
    GstElement *depay = gst_element_factory_make(codec.gstDepay.toLatin1().data(), nullptr);
    if (!depay) {
        qFatal("Failed to create depayloader");
        return;
    }

    GstElement *decoder = gst_element_factory_make(codec.gstDec.toLatin1().data(), nullptr);
    if (!decoder) {
        qFatal("Failed to create decoder");
        return;
    }

    GstElement *queue = gst_element_factory_make("queue", nullptr);
    if (!queue) {
        qFatal("Failed to create queue");
        return;
    }

    gst_bin_add_many(GST_BIN(decoderBin), depay, decoder, queue, nullptr);

    if (!gst_ghost_pad_set_target(GST_GHOST_PAD(internalReceivePad), gst_element_get_static_pad(depay, "sink")) ||
        gst_pad_link(pad, internalReceivePad) != GST_PAD_LINK_OK ||
        !gst_element_link_many(depay, decoder, queue, nullptr) ||
        !gst_ghost_pad_set_target(GST_GHOST_PAD(receivePad), gst_element_get_static_pad(queue, "src"))) {
        qFatal("Could not link all decoder pads");
        return;
    }

    gst_element_sync_state_with_parent(decoderBin);

    if (receivePadCB) {
        receivePadCB(receivePad);
    }
}

void QXmppCallStreamPrivate::addRtpSender(GstPad *pad)
{
    if (!gst_bin_add(GST_BIN(iceSendBin), apprtpsink)) {
        qFatal("Failed to add rtp sink to send bin");
    }
    gst_element_sync_state_with_parent(apprtpsink);
    if (!gst_ghost_pad_set_target(GST_GHOST_PAD(internalRtpPad), gst_element_get_static_pad(apprtpsink, "sink")) ||
        gst_pad_link(pad, internalRtpPad) != GST_PAD_LINK_OK) {
        qFatal("Failed to link rtp pads");
    }
}

void QXmppCallStreamPrivate::addRtcpSender(GstPad *pad)
{
    if (!gst_bin_add(GST_BIN(iceSendBin), apprtcpsink)) {
        qFatal("Failed to add rtcp sink to send bin");
    }
    gst_element_sync_state_with_parent(apprtcpsink);
    if (!gst_ghost_pad_set_target(GST_GHOST_PAD(internalRtcpPad), gst_element_get_static_pad(apprtcpsink, "sink")) ||
        gst_pad_link(pad, internalRtcpPad) != GST_PAD_LINK_OK) {
        qFatal("Failed to link rtcp pads");
    }
}

QXmppCallStream::QXmppCallStream(GstElement *pipeline, GstElement *rtpbin,
                                 QString media, QString creator, QString name, int id)
{
    d = new QXmppCallStreamPrivate(this, pipeline, rtpbin, media, creator, name, id);
}

QString QXmppCallStream::creator() const
{
    return d->creator;
}

QString QXmppCallStream::media() const
{
    return d->media;
}

QString QXmppCallStream::name() const
{
    return d->name;
}

int QXmppCallStream::id() const
{
    return d->id;
}

void QXmppCallStream::setReceivePadCallback(std::function<void(GstPad *)> cb)
{
    d->receivePadCB = cb;
    if (d->receivePad) {
        d->receivePadCB(d->receivePad);
    }
}

void QXmppCallStream::setSendPadCallback(std::function<void(GstPad *)> cb)
{
    d->sendPadCB = cb;
    if (d->sendPad) {
        d->sendPadCB(d->sendPad);
    }
}
