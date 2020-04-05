/*
 * Copyright (C) 2008-2020 The QXmpp developers
 *
 * Author:
 *  Jeremy Lainé
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

#include "QXmppRosterIq.h"

#include "util.h"
#include <QObject>

class tst_QXmppRosterIq : public QObject
{
    Q_OBJECT

private slots:
    void testItem_data();
    void testItem();
    void testApproved_data();
    void testApproved();
    void testVersion_data();
    void testVersion();
    void testMixAnnotate();
    void testMixChannel();
};

void tst_QXmppRosterIq::testItem_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("subscriptionType");
    QTest::addColumn<bool>("approved");

    QTest::newRow("none")
        << QByteArray(R"(<item jid="foo@example.com" subscription="none" approved="true"/>)")
        << ""
        << int(QXmppRosterIq::Item::None)
        << true;
    QTest::newRow("from")
        << QByteArray(R"(<item jid="foo@example.com" subscription="from"/>)")
        << ""
        << int(QXmppRosterIq::Item::From)
        << false;
    QTest::newRow("to")
        << QByteArray(R"(<item jid="foo@example.com" subscription="to"/>)")
        << ""
        << int(QXmppRosterIq::Item::To)
        << false;
    QTest::newRow("both")
        << QByteArray(R"(<item jid="foo@example.com" subscription="both"/>)")
        << ""
        << int(QXmppRosterIq::Item::Both)
        << false;
    QTest::newRow("remove")
        << QByteArray(R"(<item jid="foo@example.com" subscription="remove"/>)")
        << ""
        << int(QXmppRosterIq::Item::Remove)
        << false;
    QTest::newRow("notset")
        << QByteArray("<item jid=\"foo@example.com\"/>")
        << ""
        << int(QXmppRosterIq::Item::NotSet)
        << false;

    QTest::newRow("name")
        << QByteArray(R"(<item jid="foo@example.com" name="foo bar"/>)")
        << "foo bar"
        << int(QXmppRosterIq::Item::NotSet)
        << false;
}

void tst_QXmppRosterIq::testItem()
{
    QFETCH(QByteArray, xml);
    QFETCH(QString, name);
    QFETCH(int, subscriptionType);
    QFETCH(bool, approved);

    QXmppRosterIq::Item item;
    parsePacket(item, xml);
    QCOMPARE(item.bareJid(), QLatin1String("foo@example.com"));
    QCOMPARE(item.groups(), QSet<QString>());
    QCOMPARE(item.name(), name);
    QCOMPARE(int(item.subscriptionType()), subscriptionType);
    QCOMPARE(item.subscriptionStatus(), QString());
    QCOMPARE(item.isApproved(), approved);
    serializePacket(item, xml);

    item = QXmppRosterIq::Item();
    item.setBareJid("foo@example.com");
    item.setName(name);
    item.setSubscriptionType(QXmppRosterIq::Item::SubscriptionType(subscriptionType));
    item.setIsApproved(approved);
    serializePacket(item, xml);
}

void tst_QXmppRosterIq::testApproved_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<bool>("approved");

    QTest::newRow("true") << QByteArray(R"(<item jid="foo@example.com" approved="true"/>)") << true;
    QTest::newRow("1") << QByteArray(R"(<item jid="foo@example.com" approved="1"/>)") << true;
    QTest::newRow("false") << QByteArray(R"(<item jid="foo@example.com" approved="false"/>)") << false;
    QTest::newRow("0") << QByteArray(R"(<item jid="foo@example.com" approved="0"/>)") << false;
    QTest::newRow("empty") << QByteArray(R"(<item jid="foo@example.com"/>)") << false;
}

void tst_QXmppRosterIq::testApproved()
{
    QFETCH(QByteArray, xml);
    QFETCH(bool, approved);

    QXmppRosterIq::Item item;
    parsePacket(item, xml);
    QCOMPARE(item.isApproved(), approved);
}

void tst_QXmppRosterIq::testVersion_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<QString>("version");

    QTest::newRow("noversion")
        << QByteArray(R"(<iq id="woodyisacat" to="woody@zam.tw/cat" type="result"><query xmlns="jabber:iq:roster"/></iq>)")
        << "";

    QTest::newRow("version")
        << QByteArray(R"(<iq id="woodyisacat" to="woody@zam.tw/cat" type="result"><query xmlns="jabber:iq:roster" ver="3345678"/></iq>)")
        << "3345678";
}

void tst_QXmppRosterIq::testVersion()
{
    QFETCH(QByteArray, xml);
    QFETCH(QString, version);

    QXmppRosterIq iq;
    parsePacket(iq, xml);
    QCOMPARE(iq.version(), version);
    serializePacket(iq, xml);
}

void tst_QXmppRosterIq::testMixAnnotate()
{
    const QByteArray xml(
        "<iq from=\"juliet@example.com/balcony\" "
            "type=\"get\">"
            "<query xmlns=\"jabber:iq:roster\">"
                "<annotate xmlns=\"urn:xmpp:mix:roster:0\"/>"
            "</query>"
        "</iq>"
    );

    QXmppRosterIq iq;
    parsePacket(iq, xml);
    QCOMPARE(iq.mixAnnotate(), true);
    serializePacket(iq, xml);

    iq.setMixAnnotate(false);
    QCOMPARE(iq.mixAnnotate(), false);
}

void tst_QXmppRosterIq::testMixChannel()
{
    const QByteArray xml(
        "<item jid=\"balcony@example.net\">"
            "<channel xmlns=\"urn:xmpp:mix:roster:0\" participant-id=\"123456\"/>"
        "</item>"
    );

    QXmppRosterIq::Item item;
    parsePacket(item, xml);
    QCOMPARE(item.isMixChannel(), true);
    QCOMPARE(item.mixParticipantId(), QString("123456"));
    serializePacket(item, xml);

    item.setIsMixChannel(false);
    QCOMPARE(item.isMixChannel(), false);
    item.setMixParticipantId("23a7n");
    QCOMPARE(item.mixParticipantId(), QString("23a7n"));
}

QTEST_MAIN(tst_QXmppRosterIq)
#include "tst_qxmpprosteriq.moc"
