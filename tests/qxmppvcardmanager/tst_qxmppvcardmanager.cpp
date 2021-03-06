/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Authors:
 *  Melvin Keskin
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

#include "QXmppClient.h"
#include "QXmppVCardIq.h"
#include "QXmppVCardManager.h"

#include <memory>

#include "IntegrationTesting.h"
#include "util.h"
#include <QObject>

Q_DECLARE_METATYPE(QXmppVCardIq);

class tst_QXmppVCardManager : public QObject
{
    Q_OBJECT

private:
    Q_SLOT void testHandleStanza_data();
    Q_SLOT void testHandleStanza();

    // integration tests
    Q_SLOT void testSetClientVCard();

    QXmppClient m_client;
};

void tst_QXmppVCardManager::testHandleStanza_data()
{
    QTest::addColumn<QXmppVCardIq>("expectedIq");
    QTest::addColumn<bool>("isClientVCard");

#define ROW(name, iq, clientVCard) \
    QTest::newRow(QT_STRINGIFY(name)) << iq << clientVCard

    QXmppVCardIq iq;
    iq.setType(QXmppIq::Result);
    iq.setTo("stpeter@jabber.org/roundabout");
    iq.setFullName("Jeremie Miller");

    auto iqFromBare = iq;
    iqFromBare.setFrom("stpeter@jabber.org");

    auto iqFromFull = iq;
    iqFromFull.setFrom("stpeter@jabber.org/roundabout");

    ROW(client-vcard-from-empty, iq, true);
    ROW(client-vcard-from-bare, iqFromBare, true);
    ROW(client-vcard-from-full, iqFromFull, false);

#undef ROW
}

void tst_QXmppVCardManager::testHandleStanza()
{
    QFETCH(QXmppVCardIq, expectedIq);
    QFETCH(bool, isClientVCard);

    // initialize new manager to clear internal values
    QXmppVCardManager *manager = new QXmppVCardManager();
    m_client.addExtension(manager);

    // sets own jid internally
    m_client.connectToServer("stpeter@jabber.org", {});
    m_client.disconnectFromServer();

    bool vCardReceived = false;
    bool clientVCardReceived = false;

    QObject context;
    connect(manager, &QXmppVCardManager::vCardReceived, &context, [&](QXmppVCardIq iq) {
        vCardReceived = true;
        QCOMPARE(iq, expectedIq);
    });
    connect(manager, &QXmppVCardManager::clientVCardReceived, &context, [&]() {
        clientVCardReceived = true;
        QCOMPARE(manager->clientVCard(), expectedIq);
    });

    bool accepted = manager->handleStanza(writePacketToDom(expectedIq));

    QVERIFY(accepted);
    QVERIFY(vCardReceived);
    QCOMPARE(clientVCardReceived, isClientVCard);

    // clean up (client deletes manager)
    m_client.removeExtension(manager);
}

void tst_QXmppVCardManager::testSetClientVCard()
{
    SKIP_IF_INTEGRATION_TESTS_DISABLED();

    auto client = std::make_unique<QXmppClient>();
    auto *vCardManager = client->findExtension<QXmppVCardManager>();
    auto config = IntegrationTests::clientConfiguration();

    QSignalSpy connectSpy(client.get(), &QXmppClient::connected);
    QSignalSpy disconnectSpy(client.get(), &QXmppClient::disconnected);
    QSignalSpy vCardSpy(vCardManager, &QXmppVCardManager::clientVCardReceived);

    // connect to server
    client->connectToServer(config);
    QVERIFY2(connectSpy.wait(), "Could not connect to server!");

    // request own vcard
    vCardManager->requestClientVCard();
    QVERIFY(vCardSpy.wait());

    // check our vcard has the correct address
    QCOMPARE(vCardManager->clientVCard().from(), client->configuration().jidBare());

    // set a new vcard
    QXmppVCardIq newVCard;
    newVCard.setFirstName(QStringLiteral("Bob"));
    newVCard.setBirthday(QDate(1, 2, 2000));
    newVCard.setEmail(QStringLiteral("bob@qxmpp.org"));
    vCardManager->setClientVCard(newVCard);

    // there's currently no signal to see whether the change was successful...

    QCoreApplication::processEvents();

    // reconnect
    client->disconnectFromServer();
    QVERIFY(disconnectSpy.wait());

    client->connectToServer(config);
    QVERIFY2(connectSpy.wait(), "Could not connect to server!");

    // request own vcard
    vCardManager->requestClientVCard();
    QVERIFY(vCardSpy.wait());

    // check our vcard has been changed successfully
    QCOMPARE(vCardManager->clientVCard().from(), client->configuration().jidBare());
    QCOMPARE(vCardManager->clientVCard().firstName(), QStringLiteral("Bob"));
    QCOMPARE(vCardManager->clientVCard().birthday(), QDate(01, 02, 2000));
    QCOMPARE(vCardManager->clientVCard().email(), QStringLiteral("bob@qxmpp.org"));

    // reset the vcard for future tests
    vCardManager->setClientVCard(QXmppVCardIq());

    // disconnect
    client->disconnectFromServer();
    QVERIFY(disconnectSpy.wait());
}

QTEST_MAIN(tst_QXmppVCardManager)
#include "tst_qxmppvcardmanager.moc"
