/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Author:
 *  Germán Márquez Mejía
 *  Melvin Keskin
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

#include "QXmppConstants_p.h"
#include "QXmppOmemoDeviceBundle.h"
#include "QXmppOmemoDeviceElement.h"
#include "QXmppOmemoDeviceList.h"
#include "QXmppOmemoElement.h"
#include "QXmppOmemoEnvelope.h"
#include "QXmppUtils.h"

#include <QDomElement>
#include <QMap>

///
/// \class QXmppOmemoDeviceElement
///
/// \brief The QXmppOmemoDeviceElement class represents an element of the
/// OMEMO device list as defined by \xep{0384, OMEMO Encryption}.
///
/// \since QXmpp 1.5
///

class QXmppOmemoDeviceElementPrivate : public QSharedData
{
public:
    uint32_t id = 0;
    QString label;
};

///
/// Constructs an OMEMO device element.
///
QXmppOmemoDeviceElement::QXmppOmemoDeviceElement()
    : d(new QXmppOmemoDeviceElementPrivate)
{
}

///
/// Constructs a copy of \a other.
///
/// \param other
///
QXmppOmemoDeviceElement::QXmppOmemoDeviceElement(const QXmppOmemoDeviceElement &other) = default;

QXmppOmemoDeviceElement::~QXmppOmemoDeviceElement() = default;

///
/// Assigns \a other to this OMEMO device element.
///
/// \param other
///
QXmppOmemoDeviceElement &QXmppOmemoDeviceElement::operator=(const QXmppOmemoDeviceElement &other) = default;

///
/// Returns true if the IDs of both elements match.
///
bool QXmppOmemoDeviceElement::operator==(const QXmppOmemoDeviceElement &other) const
{
    return d->id == other.id();
}

///
/// Returns the ID of this device element.
///
/// The ID is used to identify a device and fetch its bundle.
/// The ID is 0 if it is unset.
///
/// \see QXmppOmemoDeviceBundle
///
/// \return this device element's ID
///
uint32_t QXmppOmemoDeviceElement::id() const
{
    return d->id;
}

///
/// Sets the ID of this device element.
///
/// A valid ID must be at least 1 and at most 2^32-1.
///
/// \param id this device element's ID
///
void QXmppOmemoDeviceElement::setId(const uint32_t id)
{
    d->id = id;
}

///
/// Returns the label of this device element.
///
/// The label is a human-readable string used to identify the device by users.
/// If no label is set, a default-constructed QString is returned.
///
/// \return this device element's label
///
QString QXmppOmemoDeviceElement::label() const
{
    return d->label;
}

///
/// Sets the optional label of this device element.
///
/// The label should not contain more than 53 characters.
///
/// \param label this device element's label
///
void QXmppOmemoDeviceElement::setLabel(const QString &label)
{
    d->label = label;
}

/// \cond
void QXmppOmemoDeviceElement::parse(const QDomElement &element)
{
    d->id = element.attribute("id").toInt();
    d->label = element.attribute("label");
}

void QXmppOmemoDeviceElement::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("device");

    writer->writeAttribute("id", QString::number(d->id));
    if (!d->label.isEmpty()) {
        writer->writeAttribute("label", d->label);
    }

    writer->writeEndElement();  // device
}
/// \endcond

///
/// Determines whether the given DOM element is an OMEMO device element.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO device element, otherwise false
///
bool QXmppOmemoDeviceElement::isOmemoDeviceElement(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("device") &&
        element.namespaceURI() == ns_omemo_2;
}

///
/// \class QXmppOmemoDeviceList
///
/// \brief The QXmppOmemoDeviceList class represents an OMEMO device list
/// as defined by \xep{0384, OMEMO Encryption}.
///
/// \since QXmpp 1.5
///

///
/// Constructs an OMEMO device list.
///
QXmppOmemoDeviceList::QXmppOmemoDeviceList()
{
}

///
/// Constructs a copy of \a other.
///
/// \param other
///
QXmppOmemoDeviceList::QXmppOmemoDeviceList(const QXmppOmemoDeviceList &other) = default;

QXmppOmemoDeviceList::~QXmppOmemoDeviceList() = default;

///
/// Assigns \a other to this OMEMO device list.
///
/// \param other
///
QXmppOmemoDeviceList &QXmppOmemoDeviceList::operator=(const QXmppOmemoDeviceList &other) = default;

/// \cond
void QXmppOmemoDeviceList::parse(const QDomElement &element)
{
    for (auto device = element.firstChildElement("device");
         !device.isNull();
         device = device.nextSiblingElement("device")) {
        QXmppOmemoDeviceElement deviceElement;
        deviceElement.parse(device);
        append(deviceElement);
    }
}

void QXmppOmemoDeviceList::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("devices");
    writer->writeDefaultNamespace(ns_omemo_2);

    for (const auto &device : *this) {
        device.toXml(writer);
    }

    writer->writeEndElement();
}
/// \endcond

///
/// Determines whether the given DOM element is an OMEMO device list.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO device list, otherwise false
///
bool QXmppOmemoDeviceList::isOmemoDeviceList(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("devices") &&
        element.namespaceURI() == ns_omemo_2;
}

///
/// \class QXmppOmemoDeviceBundle
///
/// \brief The QXmppOmemoDeviceBundle class represents an OMEMO bundle as
/// defined by \xep{0384, OMEMO Encryption}.
///
/// It is a collection of publicly accessible data used by the X3DH key exchange.
/// The data is used to build an encrypted session with an OMEMO device.
///
/// \since QXmpp 1.5
///

class QXmppOmemoDeviceBundlePrivate : public QSharedData
{
public:
    QByteArray publicIdentityKey;
    QByteArray signedPublicPreKey;
    uint32_t signedPublicPreKeyId = 0;
    QByteArray signedPublicPreKeySignature;
    QMap<uint32_t, QByteArray> publicPreKeys;
};

///
/// Constructs an OMEMO device bundle.
///
QXmppOmemoDeviceBundle::QXmppOmemoDeviceBundle()
    : d(new QXmppOmemoDeviceBundlePrivate)
{
}

///
/// Constructs a copy of \a other.
///
/// \param other
///
QXmppOmemoDeviceBundle::QXmppOmemoDeviceBundle(const QXmppOmemoDeviceBundle &other) = default;

QXmppOmemoDeviceBundle::~QXmppOmemoDeviceBundle() = default;

///
/// Assigns \a other to this OMEMO device bundle.
///
/// \param other
///
QXmppOmemoDeviceBundle &QXmppOmemoDeviceBundle::operator=(const QXmppOmemoDeviceBundle &other) = default;

///
/// Returns the public identity key.
///
/// The public identity key is the public long-term key which never changes.
///
/// \return the public identity key
///
QByteArray QXmppOmemoDeviceBundle::publicIdentityKey() const
{
    return d->publicIdentityKey;
}

///
/// Sets the public identity key.
///
/// \param key public identity key
///
void QXmppOmemoDeviceBundle::setPublicIdentityKey(const QByteArray &key)
{
    d->publicIdentityKey = key;
}

///
/// Returns the public pre key that is signed.
///
/// \return the signed public pre key
///
QByteArray QXmppOmemoDeviceBundle::signedPublicPreKey() const
{
    return d->signedPublicPreKey;
}

///
/// Sets the public pre key that is signed.
///
/// \param key signed public pre key
///
void QXmppOmemoDeviceBundle::setSignedPublicPreKey(const QByteArray &key)
{
    d->signedPublicPreKey = key;
}

///
/// Returns the ID of the public pre key that is signed.
///
/// The ID is 0 if it is unset.
///
/// \return the ID of the signed public pre key
///
uint32_t QXmppOmemoDeviceBundle::signedPublicPreKeyId() const
{
    return d->signedPublicPreKeyId;
}

///
/// Sets the ID of the public pre key that is signed.
///
/// A valid ID must be at least 1 and at most 2^32-1.
///
/// \param id ID of the signed public pre key
///
void QXmppOmemoDeviceBundle::setSignedPublicPreKeyId(const uint32_t id)
{
    d->signedPublicPreKeyId = id;
}

///
/// Returns the signature of the public pre key that is signed.
///
/// \return the signature of the signed public pre key
///
QByteArray QXmppOmemoDeviceBundle::signedPublicPreKeySignature() const
{
    return d->signedPublicPreKeySignature;
}

///
/// Returns the signature of the public pre key that is signed.
///
/// \param signature signature of the signed public pre key
///
void QXmppOmemoDeviceBundle::setSignedPublicPreKeySignature(const QByteArray &signature)
{
    d->signedPublicPreKeySignature = signature;
}

///
/// Returns the public pre keys.
///
/// The key of a key-value pair represents the ID of the corresponding public
/// pre key.
/// The value of a key-value pair represents the public pre key.
///
/// \return the public pre keys
///
QMap<uint32_t, QByteArray> QXmppOmemoDeviceBundle::publicPreKeys() const
{
    return d->publicPreKeys;
}

///
/// Sets the public pre keys.
///
/// The key of a key-value pair represents the ID of the corresponding public
/// pre key.
/// The ID must be at least 1 and at most 2^32-1, otherwise the corresponding
/// key-value pair is ignored.
/// The value of a key-value pair represents the public pre key.
///
/// \param keys public pre keys
///
void QXmppOmemoDeviceBundle::setPublicPreKeys(const QMap<uint32_t, QByteArray> &keys)
{
    for (auto it = keys.cbegin(); it != keys.cend(); it++) {
        if (it.key() > 0) {
            d->publicPreKeys.insert(it.key(), it.value());
        }
    }
}

/// \cond
void QXmppOmemoDeviceBundle::parse(const QDomElement &element)
{
    d->publicIdentityKey = QByteArray::fromBase64(element.firstChildElement(QStringLiteral("ik")).text().toLatin1());

    const auto signedPublicPreKeyElement = element.firstChildElement(QStringLiteral("spk"));
    if (!signedPublicPreKeyElement.isNull()) {
        d->signedPublicPreKeyId = signedPublicPreKeyElement.attribute(QStringLiteral("id")).toInt();
        d->signedPublicPreKey = QByteArray::fromBase64(signedPublicPreKeyElement.text().toLatin1());
    }
    d->signedPublicPreKeySignature = QByteArray::fromBase64(element.firstChildElement(QStringLiteral("spks")).text().toLatin1());

    const auto publicPreKeysElement = element.firstChildElement(QStringLiteral("prekeys"));
    if (!publicPreKeysElement.isNull()) {
        for (QDomElement publicPreKeyElement = publicPreKeysElement.firstChildElement(QStringLiteral("pk"));
             !publicPreKeyElement.isNull();
             publicPreKeyElement = publicPreKeyElement.nextSiblingElement(QStringLiteral("pk"))) {
            d->publicPreKeys.insert(publicPreKeyElement.attribute(QStringLiteral("id")).toInt(), QByteArray::fromBase64(publicPreKeyElement.text().toLatin1()));
        }
    }
}

void QXmppOmemoDeviceBundle::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("bundle"));
    writer->writeDefaultNamespace(ns_omemo_2);

    writer->writeStartElement(QStringLiteral("ik"));
    writer->writeCharacters(publicIdentityKey().toBase64());
    writer->writeEndElement();

    writer->writeStartElement(QStringLiteral("spk"));
    writer->writeAttribute(QStringLiteral("id"), QString::number(signedPublicPreKeyId()));
    writer->writeCharacters(signedPublicPreKey().toBase64());
    writer->writeEndElement();

    writer->writeStartElement(QStringLiteral("spks"));
    writer->writeCharacters(signedPublicPreKeySignature().toBase64());
    writer->writeEndElement();

    writer->writeStartElement(QStringLiteral("prekeys"));
    for (auto it = d->publicPreKeys.cbegin(); it != d->publicPreKeys.cend(); it++) {
        writer->writeStartElement(QStringLiteral("pk"));
        writer->writeAttribute(QStringLiteral("id"), QString::number(it.key()));
        writer->writeCharacters(it.value().toBase64());
        writer->writeEndElement();
    }
    writer->writeEndElement();  // prekeys

    writer->writeEndElement();  // bundle
}
/// \endcond

///
/// Determines whether the given DOM element is an OMEMO device bundle.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO device bundle, otherwise false
///
bool QXmppOmemoDeviceBundle::isOmemoDeviceBundle(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("bundle") &&
        element.namespaceURI() == ns_omemo_2;
}

///
/// \class QXmppOmemoEnvelope
///
/// \brief The QXmppOmemoEnvelope class represents an OMEMO envelope as
/// defined by \xep{0384, OMEMO Encryption}.
///
/// \since QXmpp 1.5
///

class QXmppOmemoEnvelopePrivate : public QSharedData
{
public:
    uint32_t recipientDeviceId = 0;
    bool isUsedForKeyExchange = false;
    QByteArray data;
};

///
/// Constructs an OMEMO envelope.
///
QXmppOmemoEnvelope::QXmppOmemoEnvelope()
    : d(new QXmppOmemoEnvelopePrivate)
{
}

///
/// Constructs a copy of \a other.
///
/// \param other
///
QXmppOmemoEnvelope::QXmppOmemoEnvelope(const QXmppOmemoEnvelope &other) = default;

QXmppOmemoEnvelope::~QXmppOmemoEnvelope() = default;

///
/// Assigns \a other to this OMEMO envelope.
///
/// \param other
///
QXmppOmemoEnvelope &QXmppOmemoEnvelope::operator=(const QXmppOmemoEnvelope &other) = default;

///
/// Returns the ID of the recipient's device.
///
/// The ID is 0 if it is unset.
///
/// \return the recipient's device ID
///
uint32_t QXmppOmemoEnvelope::recipientDeviceId() const
{
    return d->recipientDeviceId;
}

///
/// Sets the ID of the recipient's device.
///
/// A valid ID must be at least 1 and at most 2^32-1.
///
/// \param id recipient's device ID
///
void QXmppOmemoEnvelope::setRecipientDeviceId(const uint32_t id)
{
    d->recipientDeviceId = id;
}

///
/// Returns true if a pre-key was used to prepare this envelope.
///
/// The default is false.
///
/// \return true if a pre-key was used to prepare this envelope, otherwise false
///
bool QXmppOmemoEnvelope::isUsedForKeyExchange() const
{
    return d->isUsedForKeyExchange;
}

///
/// Sets whether a pre-key was used to prepare this envelope.
///
/// \param isUsed whether a pre-key was used to prepare this envelope
///
void QXmppOmemoEnvelope::setIsUsedForKeyExchange(const bool isUsed)
{
    d->isUsedForKeyExchange = isUsed;
}

///
/// Returns the BLOB containing the data for the underlying double ratchet library.
///
/// It should be treated like an obscure BLOB being passed as is to the ratchet
/// library for further processing.
///
/// \return the binary data for the ratchet library
///
QByteArray QXmppOmemoEnvelope::data() const
{
    return d->data;
}

///
/// Sets the BLOB containing the data from the underlying double ratchet library.
///
/// It should be treated like an obscure BLOB produced by the ratchet library.
///
/// \param data binary data from the ratchet library
///
void QXmppOmemoEnvelope::setData(const QByteArray &data)
{
    d->data = data;
}

/// \cond
void QXmppOmemoEnvelope::parse(const QDomElement &element)
{
    d->recipientDeviceId = element.attribute("rid").toInt();

    const auto isUsedForKeyExchange = element.attribute("kex");
    if (isUsedForKeyExchange == "true" || isUsedForKeyExchange == "1") {
        d->isUsedForKeyExchange = true;
    }

    d->data = QByteArray::fromBase64(element.text().toLatin1());
}

void QXmppOmemoEnvelope::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("key");
    writer->writeAttribute("rid", QString::number(d->recipientDeviceId));

    if (d->isUsedForKeyExchange) {
        helperToXmlAddAttribute(writer, "kex", "true");
    }

    writer->writeCharacters(d->data.toBase64());
    writer->writeEndElement();
}
/// \endcond

///
/// Determines whether the given DOM element is an OMEMO envelope.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO envelope, otherwise false
///
bool QXmppOmemoEnvelope::isOmemoEnvelope(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("key") &&
        element.namespaceURI() == ns_omemo_2;
}

///
/// \class QXmppOmemoElement
///
/// \brief The QXmppOmemoElement class represents an OMEMO element as
/// defined by \xep{0384, OMEMO Encryption}.
///
/// \since QXmpp 1.5
///

class QXmppOmemoElementPrivate : public QSharedData
{
public:
    uint32_t senderDeviceId = 0;
    QByteArray payload;
    QMultiMap<QString, QXmppOmemoEnvelope> envelopes;
};

///
/// Constructs an OMEMO element.
///
QXmppOmemoElement::QXmppOmemoElement()
    : d(new QXmppOmemoElementPrivate)
{
}

///
/// Constructs a copy of \a other.
///
/// \param other
///
QXmppOmemoElement::QXmppOmemoElement(const QXmppOmemoElement &other) = default;

QXmppOmemoElement::~QXmppOmemoElement() = default;

///
/// Assigns \a other to this OMEMO element.
///
/// \param other
///
QXmppOmemoElement &QXmppOmemoElement::operator=(const QXmppOmemoElement &other) = default;

///
/// Returns the ID of the sender's device.
///
/// The ID is 0 if it is unset.
///
/// \return the sender's device ID
///
uint32_t QXmppOmemoElement::senderDeviceId() const
{
    return d->senderDeviceId;
}

///
/// Sets the ID of the sender's device.
///
/// A valid ID must be at least 1 and at most 2^32-1.
///
/// \param id sender's device ID
///
void QXmppOmemoElement::setSenderDeviceId(const uint32_t id)
{
    d->senderDeviceId = id;
}

///
/// Returns the payload which consists of the encrypted SCE envelope.
///
/// \return the encrypted payload
///
QByteArray QXmppOmemoElement::payload() const
{
    return d->payload;
}

///
/// Sets the payload which consists of the encrypted SCE envelope.
///
/// \param payload encrypted payload
///
void QXmppOmemoElement::setPayload(const QByteArray &payload)
{
    d->payload = payload;
}

///
/// Searches for an OMEMO envelope by its recipient JID and device ID.
///
/// \param recipientJid bare JID of the recipient
/// \param recipientDeviceId ID of the recipient's device
///
/// \return the found OMEMO envelope
///
std::optional<QXmppOmemoEnvelope> QXmppOmemoElement::searchEnvelope(const QString &recipientJid, uint32_t recipientDeviceId) const
{
    for (auto itr = d->envelopes.constFind(recipientJid); itr != d->envelopes.constEnd() && itr.key() == recipientJid; ++itr) {
        const auto &envelope = itr.value();
        if (envelope.recipientDeviceId() == recipientDeviceId) {
            return envelope;
        }
    }

    return std::nullopt;
}

///
/// Adds an OMEMO envelope.
///
/// If a full JID is passed as recipientJid, it is converted into a bare JID.
///
/// \see QXmppOmemoEnvelope
///
/// \param recipientJid bare JID of the recipient
/// \param envelope OMEMO envelope
///
void QXmppOmemoElement::addEnvelope(const QString &recipientJid, QXmppOmemoEnvelope &envelope)
{
    d->envelopes.insert(QXmppUtils::jidToBareJid(recipientJid), envelope);
}

/// \cond
void QXmppOmemoElement::parse(const QDomElement &element)
{
    const auto header = element.firstChildElement("header");

    d->senderDeviceId = header.attribute("sid").toInt();

    for (auto recipient = header.firstChildElement("keys");
         !recipient.isNull();
         recipient = recipient.nextSiblingElement("keys")) {
        const auto recipientJid = recipient.attribute("jid");

        for (auto envelope = recipient.firstChildElement("key");
             !envelope.isNull();
             envelope = envelope.nextSiblingElement("key")) {
            QXmppOmemoEnvelope omemoEnvelope;
            omemoEnvelope.parse(envelope);
            addEnvelope(recipientJid, omemoEnvelope);
        }
    }

    d->payload = QByteArray::fromBase64(element.firstChildElement("payload").text().toLatin1());
}

void QXmppOmemoElement::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("encrypted");
    writer->writeAttribute("xmlns", ns_omemo_2);

    writer->writeStartElement("header");
    writer->writeAttribute("sid", QString::number(d->senderDeviceId));

    const auto recipientJids = d->envelopes.uniqueKeys();
    for (const auto &recipientJid : recipientJids) {
        writer->writeStartElement("keys");
        writer->writeAttribute("jid", recipientJid);

        for (auto itr = d->envelopes.constFind(recipientJid); itr != d->envelopes.constEnd() && itr.key() == recipientJid; ++itr) {
            const auto &envelope = itr.value();
            envelope.toXml(writer);
        }

        writer->writeEndElement();  // keys
    }

    writer->writeEndElement();  // header

    helperToXmlAddTextElement(writer, "payload", d->payload.toBase64());

    writer->writeEndElement();  // encrypted
}
/// \endcond

///
/// Determines whether the given DOM element is an OMEMO element.
///
/// \param element DOM element being checked
///
/// \return true if element is an OMEMO element, otherwise false
///
bool QXmppOmemoElement::isOmemoElement(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("encrypted") &&
        element.namespaceURI() == ns_omemo_2;
}
