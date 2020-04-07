/*
 * Copyright (C) 2008-2020 The QXmpp developers
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

#include "QXmppAttentionManager.h"
#include "QXmppClient.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMessage.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

#include "util.h"

class tst_QXmppAttentionManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testDiscoFeatures();
    void testReceived_data();
    void testReceived();
    void testRateLimiting();
    void testSendRequest();

private:
    void setOwnJid(const QString &jid);
    void addToRoster(const QString &jid);

    QXmppClient client;
    QXmppLogger logger;
    QXmppAttentionManager *manager;
};

void tst_QXmppAttentionManager::initTestCase()
{
    logger.setLoggingType(QXmppLogger::SignalLogging);
    client.setLogger(&logger);

    manager = new QXmppAttentionManager();
    client.addExtension(manager);
}

void tst_QXmppAttentionManager::testDiscoFeatures()
{
    QCOMPARE(manager->discoveryFeatures(), QStringList() << "urn:xmpp:attention:0");
}

void tst_QXmppAttentionManager::testReceived_data()
{
    QTest::addColumn<QXmppMessage>("msg");
    QTest::addColumn<bool>("accepted");
    QTest::addColumn<bool>("rateLimited");

    auto createMessage = [](const QString &from, bool attention, const QDateTime &stamp = {}) -> QXmppMessage {
        QXmppMessage msg;
        msg.setBody("Moin moin");
        msg.setFrom(from);
        msg.setAttentionRequested(attention);
        msg.setStamp(stamp);
        return msg;
    };

    QTest::newRow("basic")
        << createMessage("other-user@qxmpp.org/Qlient", true)
        << true;
    QTest::newRow("no-attention-requested")
        << createMessage("other-user@qxmpp.org/Qlient", false)
        << false;
    QTest::newRow("with-stamp")
        << createMessage("other-user@qxmpp.org/Qlient", true, QDateTime::currentDateTimeUtc())
        << false;
    QTest::newRow("own-account")
        << createMessage("me@qxmpp.org/Klient", true)
        << false;
    QTest::newRow("trusted")
        << createMessage("other-user@qxmpp.org/Klient", true)
        << true;
}

void tst_QXmppAttentionManager::testReceived()
{
    QFETCH(QXmppMessage, msg);
    QFETCH(bool, accepted);

    QObject context;
    setOwnJid("me@qxmpp.org");
    addToRoster("contact@qxmpp.org");
    bool signalCalled = false;
    bool limitedCalled = false;

    connect(manager, &QXmppAttentionManager::attentionRequested, &context, [&](const QXmppMessage &receivedMsg, bool isTrusted) {
        signalCalled = true;
        QCOMPARE(isTrusted, QXmppUtils::jidToBareJid(receivedMsg.from()) == QStringLiteral("contact@qxmpp.org"));
        QCOMPARE(receivedMsg.body(), msg.body());
    });

    connect(manager, &QXmppAttentionManager::attentionRequestRateLimited, &context, [&](const QXmppMessage &) {
        limitedCalled = true;
    });

    emit client.messageReceived(msg);

    QCOMPARE(signalCalled, accepted);
    QVERIFY(!limitedCalled);
}

void tst_QXmppAttentionManager::testRateLimiting()
{
    int count = 1e3;
    int allowed = 3;

    client.removeExtension(manager);
    manager = new QXmppAttentionManager(allowed, QTime(0, 0, 1));
    client.addExtension(manager);

    QObject context;
    setOwnJid("me@qxmpp.org");

    int signalCalled = 0;
    int rateLimitedCalled = 0;

    connect(manager, &QXmppAttentionManager::attentionRequested, &context, [&](const QXmppMessage &, bool) {
        signalCalled++;
    });

    connect(manager, &QXmppAttentionManager::attentionRequestRateLimited, &context, [&](const QXmppMessage &) {
        rateLimitedCalled++;
    });

    QXmppMessage msg;
    msg.setAttentionRequested(true);

    for (int i = 0; i < count; i++)
        emit client.messageReceived(msg);

    QCOMPARE(signalCalled, allowed);
    QCOMPARE(rateLimitedCalled, count - allowed);

    // wait 1 s + 50 ms because of QTimer precision problems
    QThread::currentThread()->usleep(1000e3 + 50e3);
    QCoreApplication::processEvents();

    for (int i = 0; i < count; i++)
        emit client.messageReceived(msg);

    QCOMPARE(signalCalled, allowed * 2);
    QCOMPARE(rateLimitedCalled, (count - allowed) * 2);
}

void tst_QXmppAttentionManager::testSendRequest()
{
    QObject context;

    bool signalCalled = false;
    connect(&logger, &QXmppLogger::message, &context, [&](QXmppLogger::MessageType type, const QString &message) {
        if (type == QXmppLogger::SentMessage) {
            signalCalled = true;

            QXmppMessage msg;
            parsePacket(msg, message.toUtf8());
            QCOMPARE(msg.type(), QXmppMessage::Chat);
            QCOMPARE(msg.id().size(), 36);
            QCOMPARE(msg.originId().size(), 36);
            QCOMPARE(msg.to(), QStringLiteral("account@qxmpp.org"));
            QCOMPARE(msg.body(), QStringLiteral("Hello"));
            QVERIFY(msg.isAttentionRequested());
        }
    });

    // the client is offline, so the message can't be sent and no id is returned
    QVERIFY(manager->requestAttention("account@qxmpp.org", "Hello").isEmpty());
    QVERIFY(signalCalled);
}

void tst_QXmppAttentionManager::setOwnJid(const QString &jid)
{
    client.connectToServer(jid, {});
    client.disconnectFromServer();
}

void tst_QXmppAttentionManager::addToRoster(const QString &jid)
{
    auto *rosterManager = client.findExtension<QXmppRosterManager>();

    QXmppRosterIq::Item newItem;
    newItem.setBareJid(jid);
    newItem.setSubscriptionType(QXmppRosterIq::Item::Both);

    QXmppRosterIq iq;
    iq.setFrom("qxmpp.org");
    iq.setType(QXmppIq::Set);
    iq.addItem(newItem);

    rosterManager->handleStanza(writePacketToDom(iq));
}

QTEST_MAIN(tst_QXmppAttentionManager)
#include "tst_qxmppattentionmanager.moc"
