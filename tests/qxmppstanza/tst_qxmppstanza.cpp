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

#include "QXmppStanza.h"

#include "util.h"
#include <QObject>

class tst_QXmppStanza : public QObject
{
    Q_OBJECT

private slots:
    void testExtendedAddress_data();
    void testExtendedAddress();

    void testErrorCases_data();
    void testErrorCases();
    void testErrorFileTooLarge();
    void testErrorRetry();
};

void tst_QXmppStanza::testExtendedAddress_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<bool>("delivered");
    QTest::addColumn<QString>("description");
    QTest::addColumn<QString>("jid");
    QTest::addColumn<QString>("type");

    QTest::newRow("simple")
        << QByteArray(R"(<address jid="foo@example.com/QXmpp" type="bcc"/>)")
        << false
        << QString()
        << QString("foo@example.com/QXmpp")
        << QString("bcc");

    QTest::newRow("full")
        << QByteArray(R"(<address delivered="true" desc="some description" jid="foo@example.com/QXmpp" type="bcc"/>)")
        << true
        << QString("some description")
        << QString("foo@example.com/QXmpp")
        << QString("bcc");
}

void tst_QXmppStanza::testExtendedAddress()
{
    QFETCH(QByteArray, xml);
    QFETCH(bool, delivered);
    QFETCH(QString, description);
    QFETCH(QString, jid);
    QFETCH(QString, type);

    QXmppExtendedAddress address;
    parsePacket(address, xml);
    QCOMPARE(address.isDelivered(), delivered);
    QCOMPARE(address.description(), description);
    QCOMPARE(address.jid(), jid);
    QCOMPARE(address.type(), type);
    serializePacket(address, xml);
}

void tst_QXmppStanza::testErrorCases_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<QXmppStanza::Error::Type>("type");
    QTest::addColumn<QXmppStanza::Error::Condition>("condition");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("redirectionUri");

#define ROW(name, xml, type, condition, text, redirect) \
    QTest::newRow(QT_STRINGIFY(name)) << QByteArrayLiteral(xml) << QXmppStanza::Error::type << QXmppStanza::Error::condition << text << redirect
#define BASIC(xml, type, condition) \
    ROW(condition, xml, type, condition, QString(), QString())

    ROW(
        empty-text,
        "<error type=\"modify\">"
        "<bad-request xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        BadRequest,
        QString(),
        QString());
    ROW(
        redirection-uri-gone,
        "<error type=\"cancel\">"
        "<gone xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\">"
        "xmpp:romeo@afterlife.example.net"
        "</gone>"
        "</error>",
        Cancel,
        Gone,
        QString(),
        "xmpp:romeo@afterlife.example.net");
    ROW(
        redirection-uri-redirect,
        "<error type=\"cancel\">"
        "<redirect xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\">"
        "xmpp:rms@afterlife.example.net"
        "</redirect>"
        "</error>",
        Cancel,
        Redirect,
        QString(),
        "xmpp:rms@afterlife.example.net");
    ROW(
        redirection-uri-empty,
        "<error type=\"cancel\">"
        "<redirect xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        Redirect,
        QString(),
        QString());
    ROW(
        policy-violation-text,
        "<error type=\"modify\">"
        "<policy-violation xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "<text xml:lang=\"en\" xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\">The used words are not allowed on this server.</text>"
        "</error>",
        Modify,
        PolicyViolation,
        "The used words are not allowed on this server.",
        QString());

    BASIC(
        "<error type=\"modify\">"
        "<bad-request xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        BadRequest);
    BASIC(
        "<error type=\"cancel\">"
        "<conflict xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        Conflict);
    BASIC(
        "<error type=\"cancel\">"
        "<feature-not-implemented xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        FeatureNotImplemented);
    BASIC(
        "<error type=\"auth\">"
        "<forbidden xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Auth,
        Forbidden);
    BASIC(
        "<error type=\"cancel\">"
        "<gone xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        Gone);
    BASIC(
        "<error type=\"cancel\">"
        "<internal-server-error xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        InternalServerError);
    BASIC(
        "<error type=\"cancel\">"
        "<item-not-found xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        ItemNotFound);
    BASIC(
        "<error type=\"modify\">"
        "<jid-malformed xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        JidMalformed);
    BASIC(
        "<error type=\"modify\">"
        "<not-acceptable xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        NotAcceptable);
    BASIC(
        "<error type=\"cancel\">"
        "<not-allowed xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        NotAllowed);
    BASIC(
        "<error type=\"auth\">"
        "<not-authorized xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Auth,
        NotAuthorized);
    BASIC(
        "<error type=\"modify\">"
        "<policy-violation xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        PolicyViolation);
    BASIC(
        "<error type=\"wait\">"
        "<recipient-unavailable xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Wait,
        RecipientUnavailable);
    BASIC(
        "<error type=\"modify\">"
        "<redirect xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        Redirect);
    BASIC(
        "<error type=\"auth\">"
        "<registration-required xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Auth,
        RegistrationRequired);
    BASIC(
        "<error type=\"cancel\">"
        "<remote-server-not-found xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        RemoteServerNotFound);
    BASIC(
        "<error type=\"wait\">"
        "<remote-server-timeout xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Wait,
        RemoteServerTimeout);
    BASIC(
        "<error type=\"wait\">"
        "<resource-constraint xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Wait,
        ResourceConstraint);
    BASIC(
        "<error type=\"cancel\">"
        "<service-unavailable xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Cancel,
        ServiceUnavailable);
    BASIC(
        "<error type=\"auth\">"
        "<subscription-required xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Auth,
        SubscriptionRequired);
    BASIC(
        "<error type=\"modify\">"
        "<undefined-condition xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "</error>",
        Modify,
        UndefinedCondition);

#undef BASIC
#undef ROW
}

void tst_QXmppStanza::testErrorCases()
{
    QFETCH(QByteArray, xml);
    QFETCH(QXmppStanza::Error::Type, type);
    QFETCH(QXmppStanza::Error::Condition, condition);
    QFETCH(QString, text);
    QFETCH(QString, redirectionUri);

    // parsing
    QXmppStanza::Error error;
    parsePacket(error, xml);
    QCOMPARE(error.type(), type);
    QCOMPARE(error.condition(), condition);
    QCOMPARE(error.text(), text);
    QCOMPARE(error.redirectionUri(), redirectionUri);
    // check parsed error results in the same xml
    serializePacket(error, xml);

    // serialization from setters
    error = QXmppStanza::Error();
    error.setType(type);
    error.setCondition(condition);
    error.setText(text);
    error.setRedirectionUri(redirectionUri);
    serializePacket(error, xml);
}

void tst_QXmppStanza::testErrorFileTooLarge()
{
    const QByteArray xml(
        "<error type=\"modify\">"
        "<not-acceptable xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "<text xml:lang=\"en\" "
        "xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\">"
        "File too large. The maximum file size is 20000 bytes"
        "</text>"
        "<file-too-large xmlns=\"urn:xmpp:http:upload:0\">"
        "<max-file-size>20000</max-file-size>"
        "</file-too-large>"
        "</error>");

    QXmppStanza::Error error;
    parsePacket(error, xml);
    QCOMPARE(error.type(), QXmppStanza::Error::Modify);
    QCOMPARE(error.text(), QString("File too large. The maximum file size is "
                                   "20000 bytes"));
    QCOMPARE(error.condition(), QXmppStanza::Error::NotAcceptable);
    QVERIFY(error.fileTooLarge());
    QCOMPARE(error.maxFileSize(), 20000);
    serializePacket(error, xml);

    // test setters
    error.setMaxFileSize(60000);
    QCOMPARE(error.maxFileSize(), 60000);
    error.setFileTooLarge(false);
    QVERIFY(!error.fileTooLarge());

    QXmppStanza::Error e2;
    e2.setMaxFileSize(123000);
    QVERIFY(e2.fileTooLarge());
}

void tst_QXmppStanza::testErrorRetry()
{
    const QByteArray xml(
        "<error type=\"wait\">"
        "<resource-constraint "
        "xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\"/>"
        "<text xml:lang=\"en\" "
        "xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\">"
        "Quota reached. You can only upload 5 files in 5 minutes"
        "</text>"
        "<retry xmlns=\"urn:xmpp:http:upload:0\" "
        "stamp=\"2017-12-03T23:42:05Z\"/>"
        "</error>");

    QXmppStanza::Error error;
    parsePacket(error, xml);
    QCOMPARE(error.type(), QXmppStanza::Error::Wait);
    QCOMPARE(error.text(), QString("Quota reached. You can only upload 5 "
                                   "files in 5 minutes"));
    QCOMPARE(error.condition(), QXmppStanza::Error::ResourceConstraint);
    QCOMPARE(error.retryDate(), QDateTime(QDate(2017, 12, 03), QTime(23, 42, 05), Qt::UTC));
    serializePacket(error, xml);

    // test setter
    error.setRetryDate(QDateTime(QDate(1985, 10, 26), QTime(1, 35)));
    QCOMPARE(error.retryDate(), QDateTime(QDate(1985, 10, 26), QTime(1, 35)));
}

QTEST_MAIN(tst_QXmppStanza)
#include "tst_qxmppstanza.moc"
