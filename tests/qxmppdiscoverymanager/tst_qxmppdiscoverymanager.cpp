// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppDiscoveryManager.h"

#include "TestClient.h"

class tst_QXmppDiscoveryManager : public QObject
{
    Q_OBJECT
private:
    Q_SLOT void testInfo();
    Q_SLOT void testItems();
};

void tst_QXmppDiscoveryManager::testInfo()
{
    TestClient test;
    auto *discoManager = test.addNewExtension<QXmppDiscoveryManager>();

    auto future = discoManager->requestDiscoInfo("user@example.org");
    test.expect("<iq id='qxmpp1' to='user@example.org' type='get'><query xmlns='http://jabber.org/protocol/disco#info'/></iq>");
    test.inject<QString>(R"(
<iq id='qxmpp1' from='user@example.org' type='result'>
    <query xmlns='http://jabber.org/protocol/disco#info'>
        <identity category='pubsub' type='service'/>
        <feature var='http://jabber.org/protocol/pubsub'/>
        <feature var='urn:xmpp:mix:core:1'/>
    </query>
</iq>)");

    const auto info = expectFutureVariant<QXmppDiscoveryIq>(future.toFuture(this));

    const QStringList expFeatures = { "http://jabber.org/protocol/pubsub", "urn:xmpp:mix:core:1" };
    QCOMPARE(info.features(), expFeatures);
    QCOMPARE(info.identities().count(), 1);
}

void tst_QXmppDiscoveryManager::testItems()
{
    TestClient test;
    auto *discoManager = test.addNewExtension<QXmppDiscoveryManager>();

    auto future = discoManager->requestDiscoItems("user@example.org");
    test.expect("<iq id='qxmpp1' to='user@example.org' type='get'><query xmlns='http://jabber.org/protocol/disco#items'/></iq>");
    qDebug() << "Moin";
    test.inject<QString>(R"(
<iq type='result'
    from='user@example.org'
    id='qxmpp1'>
  <query xmlns='http://jabber.org/protocol/disco#items'>
    <item name='368866411b877c30064a5f62b917cffe'/>
    <item name='3300659945416e274474e469a1f0154c'/>
    <item name='4e30f35051b7b8b42abe083742187228'/>
    <item name='ae890ac52d0df67ed7cfdf51b644e901'/>
  </query>
</iq>)");

    const auto items = expectFutureVariant<QList<QXmppDiscoveryIq::Item>>(future.toFuture(this));

    const QStringList expFeatures = { "http://jabber.org/protocol/pubsub", "urn:xmpp:mix:core:1" };
    QCOMPARE(items.size(), 4);
    QCOMPARE(items.at(0).name(), QStringLiteral("368866411b877c30064a5f62b917cffe"));
    QCOMPARE(items.at(1).name(), QStringLiteral("3300659945416e274474e469a1f0154c"));
    QCOMPARE(items.at(2).name(), QStringLiteral("4e30f35051b7b8b42abe083742187228"));
    QCOMPARE(items.at(3).name(), QStringLiteral("ae890ac52d0df67ed7cfdf51b644e901"));
}

QTEST_MAIN(tst_QXmppDiscoveryManager)

#include "tst_qxmppdiscoverymanager.moc"
