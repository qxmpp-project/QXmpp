/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Authors:
 *  Linus Jahn
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

#include "QXmppPubSubAffiliation.h"
#include "QXmppPubSubSubscription.h"

#include "util.h"
#include <QObject>

using Affiliation = QXmppPubSubAffiliation;
using AffiliationType = QXmppPubSubAffiliation::Affiliation;
using Subscription = QXmppPubSubSubscription;
using SubscriptionConfig = QXmppPubSubSubscription::ConfigurationSupport;
using SubscriptionState = QXmppPubSubSubscription::State;

enum PubSubNamespace {
    PubSubNs,
    PubSubEventNs,
    PubSubOwnerNs,
};
Q_DECLARE_METATYPE(PubSubNamespace)

template<typename T>
void parsePacket(T &packet, const QByteArray &xml, PubSubNamespace xmlns)
{
    QByteArray newXml;
    switch (xmlns) {
    case PubSubNs:
        newXml = "<outer xmlns='http://jabber.org/protocol/pubsub'>" + xml + "</outer>";
        break;
    case PubSubEventNs:
        newXml = "<outer xmlns='http://jabber.org/protocol/pubsub#event'>" + xml + "</outer>";
        break;
    case PubSubOwnerNs:
        newXml = "<outer xmlns='http://jabber.org/protocol/pubsub#owner'>" + xml + "</outer>";
        break;
    }
    packet.parse(xmlToDom(newXml).firstChildElement());
}

class tst_QXmppPubSub : public QObject
{
    Q_OBJECT

private:
    Q_SLOT void testAffiliation_data();
    Q_SLOT void testAffiliation();
    Q_SLOT void testIsAffiliation_data();
    Q_SLOT void testIsAffiliation();
    Q_SLOT void testSubscription_data();
    Q_SLOT void testSubscription();
};

void tst_QXmppPubSub::testAffiliation_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<AffiliationType>("type");
    QTest::addColumn<QString>("jid");
    QTest::addColumn<QString>("node");

#define ROW(name, xml, type, jid, node) \
    QTest::newRow(name) << QByteArrayLiteral(xml) << type << jid << node

    ROW("owner", "<affiliation affiliation='owner' node='node1'/>", AffiliationType::Owner, QString(), QString("node1"));
    ROW("publisher", "<affiliation affiliation='publisher' node='node2'/>", AffiliationType::Publisher, QString(), QString("node2"));
    ROW("outcast", "<affiliation affiliation='outcast' node='noise'/>", AffiliationType::Outcast, QString(), QString("noise"));
    ROW("none", "<affiliation affiliation='none' node='stuff'/>", AffiliationType::None, QString(), QString("stuff"));
    ROW("with-jid", "<affiliation affiliation='owner' jid='snob@qxmpp.org'/>", AffiliationType::Owner, QString("snob@qxmpp.org"), QString());

#undef ROW
}

void tst_QXmppPubSub::testAffiliation()
{
    QFETCH(QByteArray, xml);
    QFETCH(AffiliationType, type);
    QFETCH(QString, jid);
    QFETCH(QString, node);

    Affiliation affiliation;
    parsePacket(affiliation, xml);
    QCOMPARE(affiliation.jid(), jid);
    QCOMPARE(affiliation.node(), node);
    QCOMPARE(affiliation.type(), type);
    serializePacket(affiliation, xml);

    affiliation = {};
    affiliation.setJid(jid);
    affiliation.setNode(node);
    affiliation.setType(type);
    serializePacket(affiliation, xml);
}

void tst_QXmppPubSub::testIsAffiliation_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<bool>("accepted");

    QTest::newRow("ps-correct")
        << QByteArrayLiteral("<parent xmlns='http://jabber.org/protocol/pubsub'><affiliation affiliation=\"owner\" node=\"node1\"/></parent>")
        << true;
    QTest::newRow("ps-missing-node")
        << QByteArrayLiteral("<parent xmlns='http://jabber.org/protocol/pubsub'><affiliation affiliation=\"owner\"/></parent>")
        << false;
    QTest::newRow("ps-invalid-affiliation")
        << QByteArrayLiteral("<parent xmlns='http://jabber.org/protocol/pubsub'><affiliation affiliation=\"gigaowner\" node=\"node1\"/></parent>")
        << false;
    QTest::newRow("psowner-correct")
        << QByteArrayLiteral("<parent xmlns='http://jabber.org/protocol/pubsub#owner'><affiliation affiliation=\"owner\" jid=\"snob@qxmpp.org\"/></parent>")
        << true;
    QTest::newRow("psowner-missing-jid")
        << QByteArrayLiteral("<parent xmlns='http://jabber.org/protocol/pubsub#owner'><affiliation affiliation=\"owner\"/></parent>")
        << false;
    QTest::newRow("psowner-invalid-affiliation")
        << QByteArrayLiteral("<parent xmlns='http://jabber.org/protocol/pubsub#owner'><affiliation affiliation=\"superowner\" jid=\"snob@qxmpp.org\"/></parent>")
        << false;
    QTest::newRow("invalid-namespace")
        << QByteArrayLiteral("<parent xmlns='urn:xmpp:mix:0'><affiliation affiliation=\"owner\" node=\"node1\"/></parent>")
        << false;
}

void tst_QXmppPubSub::testIsAffiliation()
{
    QFETCH(QByteArray, xml);
    QFETCH(bool, accepted);

    auto dom = xmlToDom(xml).firstChildElement();
    QCOMPARE(Affiliation::isAffiliation(dom), accepted);
}

void tst_QXmppPubSub::testSubscription_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<PubSubNamespace>("pubSubNs");
    QTest::addColumn<SubscriptionState>("state");
    QTest::addColumn<QString>("jid");
    QTest::addColumn<QString>("node");
    QTest::addColumn<QString>("subid");
    QTest::addColumn<SubscriptionConfig>("configSupport");

#define ROW(name, xmlns, xml, state, jid, node, subid, configSupport) \
    QTest::newRow(name) << QByteArrayLiteral(xml) << xmlns << state << jid << node << subid << configSupport

    ROW("subscribed", PubSubNs, "<subscription jid='francisco@denmark.lit' node='node1' subscription='subscribed'/>", SubscriptionState::Subscribed, QString("francisco@denmark.lit"), QString("node1"), QString(), SubscriptionConfig::Unavailable);
    ROW("unconfigured", PubSubNs, "<subscription jid='francisco@denmark.lit' node='node5' subscription='unconfigured'/>", SubscriptionState::Unconfigured, QString("francisco@denmark.lit"), QString("node5"), QString(), SubscriptionConfig::Unavailable);
    ROW("subscribed-subid", PubSubNs, "<subscription jid='francisco@denmark.lit' node='node6' subscription='subscribed' subid='123-abc'/>", SubscriptionState::Subscribed, QString("francisco@denmark.lit"), QString("node6"), QString("123-abc"), SubscriptionConfig::Unavailable);
    ROW("pending", PubSubNs, "<subscription jid='francisco@denmark.lit' node='princely_musings' subscription='pending'/>", SubscriptionState::Pending, QString("francisco@denmark.lit"), QString("princely_musings"), QString(), SubscriptionConfig::Unavailable);
    ROW("config-required", PubSubNs, "<subscription jid='francisco@denmark.lit' node='princely_musings' subscription='unconfigured'><subscribe-options><required/></subscribe-options></subscription>", SubscriptionState::Unconfigured, QString("francisco@denmark.lit"), QString("princely_musings"), QString(), SubscriptionConfig::Required);
    ROW("config-available", PubSubNs, "<subscription jid='francisco@denmark.lit' node='princely_musings' subscription='unconfigured'><subscribe-options/></subscription>", SubscriptionState::Unconfigured, QString("francisco@denmark.lit"), QString("princely_musings"), QString(), SubscriptionConfig::Available);

#undef ROW
}

void tst_QXmppPubSub::testSubscription()
{
    QFETCH(QByteArray, xml);
    QFETCH(PubSubNamespace, pubSubNs);
    QFETCH(SubscriptionState, state);
    QFETCH(QString, jid);
    QFETCH(QString, node);
    QFETCH(QString, subid);
    QFETCH(SubscriptionConfig, configSupport);

    QXmppPubSubSubscription sub;
    parsePacket(sub, xml, pubSubNs);
    serializePacket(sub, xml);
    QCOMPARE(sub.state(), state);
    QCOMPARE(sub.jid(), jid);
    QCOMPARE(sub.node(), node);
    QCOMPARE(sub.subId(), subid);
    QCOMPARE(sub.configurationSupport(), configSupport);

    switch (configSupport) {
    case SubscriptionConfig::Unavailable:
        if (state == SubscriptionState::Unconfigured) {
            QVERIFY(sub.isConfigurationRequired());
        } else {
            QVERIFY(!sub.isConfigurationRequired());
        }
        QVERIFY(!sub.isConfigurationSupported());
        break;
    case SubscriptionConfig::Available:
        if (state == SubscriptionState::Unconfigured) {
            QVERIFY(sub.isConfigurationRequired());
        } else {
            QVERIFY(!sub.isConfigurationRequired());
        }
        QVERIFY(sub.isConfigurationSupported());
        break;
    case SubscriptionConfig::Required:
        QVERIFY(sub.isConfigurationRequired());
        QVERIFY(sub.isConfigurationSupported());
        break;
    }

    sub = {};
    sub.setState(state);
    sub.setJid(jid);
    sub.setNode(node);
    sub.setSubId(subid);
    sub.setConfigurationSupport(configSupport);
    serializePacket(sub, xml);
}

QTEST_MAIN(tst_QXmppPubSub)
#include "tst_qxmpppubsub.moc"
