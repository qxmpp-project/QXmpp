/*
 * Copyright (C) 2008-2012 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
 *  Jeremy Lainé
 *
 * Source:
 *  http://code.google.com/p/qxmpp
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

#include <cstdlib>

#include <QCryptographicHash>
#include <QDomElement>
#include <QStringList>
#include <QUrl>

#include "QXmppSaslAuth.h"
#include "QXmppSaslAuth_p.h"
#include "QXmppUtils.h"

const char *ns_xmpp_sasl = "urn:ietf:params:xml:ns:xmpp-sasl";

QXmppSaslStanza::QXmppSaslStanza(const QString &type, const QByteArray &value)
    : m_type(type)
    , m_value(value)
{
}

QByteArray QXmppSaslStanza::value() const
{
    return m_value;
}

void QXmppSaslStanza::setValue(const QByteArray &value)
{
    m_value = value;
}


void QXmppSaslStanza::parse(const QDomElement &element)
{
    m_type = element.nodeName();
    m_value = QByteArray::fromBase64(element.text().toAscii());
}

void QXmppSaslStanza::toXml(QXmlStreamWriter *writer) const
{
    if (!m_type.isEmpty()) {
        writer->writeStartElement(m_type);
        writer->writeAttribute("xmlns", ns_xmpp_sasl);
        if (!m_value.isEmpty())
            writer->writeCharacters(m_value.toBase64());
        writer->writeEndElement();
    }
}

class QXmppSaslClientPrivate
{
public:
    QString host;
    QString serviceType;
    QString username;
    QString password;
};

QXmppSaslClient::QXmppSaslClient(QObject *parent)
    : QXmppLoggable(parent)
    , d(new QXmppSaslClientPrivate)
{
}

QXmppSaslClient::~QXmppSaslClient()
{
    delete d;
}

/// Returns a list of supported mechanisms.

QStringList QXmppSaslClient::availableMechanisms()
{
    return QStringList() << "PLAIN" << "DIGEST-MD5" << "ANONYMOUS" << "X-FACEBOOK-PLATFORM";
}

/// Creates an SASL client for the given mechanism.

QXmppSaslClient* QXmppSaslClient::create(const QString &mechanism, QObject *parent)
{
    if (mechanism == "PLAIN") {
        return new QXmppSaslClientPlain(parent);
    } else if (mechanism == "DIGEST-MD5") {
        return new QXmppSaslClientDigestMd5(parent);
    } else if (mechanism == "ANONYMOUS") {
        return new QXmppSaslClientAnonymous(parent);
    } else if (mechanism == "X-FACEBOOK-PLATFORM") {
        return new QXmppSaslClientFacebook(parent);
    } else {
        return 0;
    }
}

/// Returns the host.

QString QXmppSaslClient::host() const
{
    return d->host;
}

/// Sets the host.

void QXmppSaslClient::setHost(const QString &host)
{
    d->host = host;
}

/// Returns the service type, e.g. "xmpp".

QString QXmppSaslClient::serviceType() const
{
    return d->serviceType;
}

/// Sets the service type, e.g. "xmpp".

void QXmppSaslClient::setServiceType(const QString &serviceType)
{
    d->serviceType = serviceType;
}

/// Returns the username.

QString QXmppSaslClient::username() const
{
    return d->username;
}

/// Sets the username.

void QXmppSaslClient::setUsername(const QString &username)
{
    d->username = username;
}

/// Returns the password.

QString QXmppSaslClient::password() const
{
    return d->password;
}

/// Sets the password.

void QXmppSaslClient::setPassword(const QString &password)
{
    d->password = password;
}

QXmppSaslClientAnonymous::QXmppSaslClientAnonymous(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientAnonymous::mechanism() const
{
    return "ANONYMOUS";
}

bool QXmppSaslClientAnonymous::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        response = QByteArray();
        m_step++;
        return true;
    } else {
        warning("QXmppSaslClientAnonymous : Invalid step");
        return false;
    }
}

QXmppSaslClientDigestMd5::QXmppSaslClientDigestMd5(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientDigestMd5::mechanism() const
{
    return "DIGEST-MD5";
}

bool QXmppSaslClientDigestMd5::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        response = QByteArray();
        m_step++;
        return true;
    } else if (m_step == 1) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(challenge);

        if (!input.contains("nonce")) {
            warning("QXmppSaslClientDigestMd5 : Invalid input on step 1");
            return false;
        }

        // determine realm
        const QByteArray realm = input.value("realm");

        // determine quality of protection
        const QList<QByteArray> qops = input.value("qop", "auth").split(',');
        if (!qops.contains("auth")) {
            warning("QXmppSaslClientDigestMd5 : Invalid quality of protection");
            return false;
        }

        m_saslDigest.setQop("auth");
        m_saslDigest.setCnonce(QXmppSaslDigestMd5::generateNonce());
        m_saslDigest.setNc("00000001");
        m_saslDigest.setDigestUri(QString("%1/%2").arg(serviceType(), host()).toUtf8());
        m_saslDigest.setNonce(input.value("nonce"));
        m_saslDigest.setSecret(QCryptographicHash::hash(
            username().toUtf8() + ":" + realm + ":" + password().toUtf8(),
            QCryptographicHash::Md5));

        // Build response
        QMap<QByteArray, QByteArray> output;
        output["username"] = username().toUtf8();
        if (!realm.isEmpty())
            output["realm"] = realm;
        output["nonce"] = m_saslDigest.nonce();
        output["qop"] = m_saslDigest.qop();
        output["cnonce"] = m_saslDigest.cnonce();
        output["nc"] = m_saslDigest.nc();
        output["digest-uri"] = m_saslDigest.digestUri();
        output["response"] = m_saslDigest.calculateDigest(
            QByteArray("AUTHENTICATE:") + m_saslDigest.digestUri());
        if (!m_saslDigest.authzid().isEmpty())
            output["authzid"] = m_saslDigest.authzid();
        output["charset"] = "utf-8";

        response = QXmppSaslDigestMd5::serializeMessage(output);
        m_step++;
        return true;
    } else if (m_step == 2) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(challenge);

        // check new challenge
        if (input.value("rspauth") !=
            m_saslDigest.calculateDigest(QByteArray(":") + m_saslDigest.digestUri())) {
            warning("QXmppSaslClientDigestMd5 : Invalid challenge on step 2");
            return false;
        }

        response = QByteArray();
        m_step++;
        return true;
    } else {
        warning("QXmppSaslClientDigestMd5 : Invalid step");
        return false;
    }
}

QXmppSaslClientFacebook::QXmppSaslClientFacebook(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientFacebook::mechanism() const
{
    return "X-FACEBOOK-PLATFORM";
}

bool QXmppSaslClientFacebook::respond(const QByteArray &challenge, QByteArray &response)
{
    if (m_step == 0) {
        // no initial response
        response = QByteArray();
        m_step++;
        return true;
    } else if (m_step == 1) {
        // parse request
        QUrl requestUrl;
        requestUrl.setEncodedQuery(challenge);
        if (!requestUrl.hasQueryItem("method") || !requestUrl.hasQueryItem("nonce")) {
            warning("QXmppSaslClientFacebook : Invalid challenge, nonce or method missing");
            return false;
        }

        // build response
        QUrl responseUrl;
        responseUrl.addQueryItem("access_token", username());
        responseUrl.addQueryItem("api_key", password());
        responseUrl.addQueryItem("call_id", 0);
        responseUrl.addQueryItem("method", requestUrl.queryItemValue("method"));
        responseUrl.addQueryItem("nonce", requestUrl.queryItemValue("nonce"));
        responseUrl.addQueryItem("v", "1.0");

        response = responseUrl.encodedQuery();
        m_step++;
        return true;
    } else {
        warning("QXmppSaslClientFacebook : Invalid step");
        return false;
    }
}

QXmppSaslClientPlain::QXmppSaslClientPlain(QObject *parent)
    : QXmppSaslClient(parent)
    , m_step(0)
{
}

QString QXmppSaslClientPlain::mechanism() const
{
    return "PLAIN";
}

bool QXmppSaslClientPlain::respond(const QByteArray &challenge, QByteArray &response)
{
    Q_UNUSED(challenge);
    if (m_step == 0) {
        response = QString('\0' + username() + '\0' + password()).toUtf8();
        m_step++;
        return true;
    } else {
        warning("QXmppSaslClientPlain : Invalid step");
        return false;
    }
}

class QXmppSaslServerPrivate
{
public:
    QString username;
    QString password;
    QString realm;
};

QXmppSaslServer::QXmppSaslServer(QObject *parent)
    : QXmppLoggable(parent)
    , d(new QXmppSaslServerPrivate)
{
}

QXmppSaslServer::~QXmppSaslServer()
{
    delete d;
}

/// Creates an SASL server for the given mechanism.

QXmppSaslServer* QXmppSaslServer::create(const QString &mechanism, QObject *parent)
{
    if (mechanism == "PLAIN") {
        return new QXmppSaslServerPlain(parent);
    } else if (mechanism == "DIGEST-MD5") {
        return new QXmppSaslServerDigestMd5(parent);
    } else if (mechanism == "ANONYMOUS") {
        return new QXmppSaslServerAnonymous(parent);
    } else {
        return 0;
    }
}

/// Returns the username.

QString QXmppSaslServer::username() const
{
    return d->username;
}

/// Sets the username.

void QXmppSaslServer::setUsername(const QString &username)
{
    d->username = username;
}

/// Returns the password.

QString QXmppSaslServer::password() const
{
    return d->password;
}

/// Sets the password.

void QXmppSaslServer::setPassword(const QString &password)
{
    d->password = password;
}

/// Returns the realm.

QString QXmppSaslServer::realm() const
{
    return d->realm;
}

/// Sets the realm.

void QXmppSaslServer::setRealm(const QString &realm)
{
    d->realm = realm;
}

QXmppSaslServerAnonymous::QXmppSaslServerAnonymous(QObject *parent)
    : QXmppSaslServer(parent)
    , m_step(0)
{
}

QString QXmppSaslServerAnonymous::mechanism() const
{
    return "ANONYMOUS";
}

QXmppSaslServer::Response QXmppSaslServerAnonymous::respond(const QByteArray &request, QByteArray &response)
{
    Q_UNUSED(request);
    if (m_step == 0) {
        m_step++;
        response = QByteArray();
        return Succeeded;
    } else {
        warning("QXmppSaslServerAnonymous : Invalid step");
        return Failed;
    }
}

QXmppSaslServerDigestMd5::QXmppSaslServerDigestMd5(QObject *parent)
    : QXmppSaslServer(parent)
    , m_step(0)
{
}

QString QXmppSaslServerDigestMd5::mechanism() const
{
    return "DIGEST-MD5";
}

QXmppSaslServer::Response QXmppSaslServerDigestMd5::respond(const QByteArray &request, QByteArray &response)
{
    if (m_step == 0) {
        // generate nonce
        m_saslDigest.setNonce(QXmppSaslDigestMd5::generateNonce());
        //m_saslDigest.setQop("auth");

        QMap<QByteArray, QByteArray> output;
        output["nonce"] = m_saslDigest.nonce();
        if (!realm().isEmpty())
            output["realm"] = realm().toUtf8();
        output["qop"] = "auth";
        output["charset"] = "utf-8";
        output["algorithm"] = "md5-sess";

        m_step++;
        response = QXmppSaslDigestMd5::serializeMessage(output);
        return Challenge;
    } else if (m_step == 1) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(request);
        const QByteArray realm = input.value("realm");

        setUsername(QString::fromUtf8(input.value("username")));
        if (password().isEmpty())
            return InputNeeded;

        m_saslDigest.setQop("auth");
        m_saslDigest.setDigestUri(input.value("digest-uri"));
        m_saslDigest.setNc(input.value("nc"));
        m_saslDigest.setCnonce(input.value("cnonce"));
        m_saslDigest.setSecret(QCryptographicHash::hash(
            username().toUtf8() + ":" + realm + ":" + password().toUtf8(),
            QCryptographicHash::Md5));

        const QByteArray expectedResponse = m_saslDigest.calculateDigest(
            QByteArray("AUTHENTICATE:") + m_saslDigest.digestUri());

        if (input.value("response") != expectedResponse) {
            return Failed;
        }

        QMap<QByteArray, QByteArray> output;
        output["rspauth"] = m_saslDigest.calculateDigest(
            QByteArray(":") + m_saslDigest.digestUri());

        m_step++;
        response = QXmppSaslDigestMd5::serializeMessage(output);
        return Challenge;
    } else if (m_step == 2) {
        m_step++;
        response = QByteArray();
        return Succeeded;
    } else {
        warning("QXmppSaslServerDigestMd5 : Invalid step");
        return Failed;
    }
}

QXmppSaslServerPlain::QXmppSaslServerPlain(QObject *parent)
    : QXmppSaslServer(parent)
    , m_step(0)
{
}

QString QXmppSaslServerPlain::mechanism() const
{
    return "PLAIN";
}

QXmppSaslServer::Response QXmppSaslServerPlain::respond(const QByteArray &request, QByteArray &response)
{
    if (m_step == 0) {
        QList<QByteArray> auth = request.split('\0');
        if (auth.size() != 3) {
            warning("QXmppSaslServerPlain : Invalid input");
            return Failed;
        }
        setUsername(QString::fromUtf8(auth[1]));
        setPassword(QString::fromUtf8(auth[2]));

        m_step++;
        response = QByteArray();
        return InputNeeded;
    } else {
        warning("QXmppSaslServerPlain : Invalid step");
        return Failed;
    }
}

QByteArray QXmppSaslDigestMd5::authzid() const
{
    return m_authzid;
}

void QXmppSaslDigestMd5::setAuthzid(const QByteArray &authzid)
{
    m_authzid = authzid;
}

QByteArray QXmppSaslDigestMd5::cnonce() const
{
    return m_cnonce;
}

void QXmppSaslDigestMd5::setCnonce(const QByteArray &cnonce)
{
    m_cnonce = cnonce;
}

QByteArray QXmppSaslDigestMd5::digestUri() const
{
    return m_digestUri;
}

void QXmppSaslDigestMd5::setDigestUri(const QByteArray &digestUri)
{
    m_digestUri = digestUri;
}

QByteArray QXmppSaslDigestMd5::nc() const
{
    return m_nc;
}

void QXmppSaslDigestMd5::setNc(const QByteArray &nc)
{
    m_nc = nc;
}

QByteArray QXmppSaslDigestMd5::nonce() const
{
    return m_nonce;
}

void QXmppSaslDigestMd5::setNonce(const QByteArray &nonce)
{
    m_nonce = nonce;
}

QByteArray QXmppSaslDigestMd5::qop() const
{
    return m_qop;
}

void QXmppSaslDigestMd5::setQop(const QByteArray &qop)
{
    m_qop = qop;
}

void QXmppSaslDigestMd5::setSecret(const QByteArray &secret)
{
    m_secret = secret;
}

QByteArray QXmppSaslDigestMd5::generateNonce()
{
    QByteArray nonce = QXmppUtils::generateRandomBytes(32);

    // The random data can the '=' char is not valid as it is a delimiter,
    // so to be safe, base64 the nonce
    return nonce.toBase64();
}

/// Calculate digest response for use with XMPP/SASL.
///
/// \param A2
///

QByteArray QXmppSaslDigestMd5::calculateDigest(const QByteArray &A2) const
{
    QByteArray ha1 = m_secret + ':' + m_nonce + ':' + m_cnonce;

    if (!m_authzid.isEmpty())
        ha1 += ':' + m_authzid;

    return calculateDigest(ha1, A2);
}

/// Calculate generic digest response.
///
/// \param A1
/// \param A2
///

QByteArray QXmppSaslDigestMd5::calculateDigest(const QByteArray &A1, const QByteArray &A2) const
{
    QByteArray HA1 = QCryptographicHash::hash(A1, QCryptographicHash::Md5).toHex();
    QByteArray HA2 = QCryptographicHash::hash(A2, QCryptographicHash::Md5).toHex();
    QByteArray KD;
    if (m_qop == "auth" || m_qop == "auth-int")
        KD = HA1 + ':' + m_nonce + ':' + m_nc + ':' + m_cnonce + ':' + m_qop + ':' + HA2;
    else
        KD = HA1 + ':' + m_nonce + ':' + HA2;
    return QCryptographicHash::hash(KD, QCryptographicHash::Md5).toHex();
}

QMap<QByteArray, QByteArray> QXmppSaslDigestMd5::parseMessage(const QByteArray &ba)
{
    QMap<QByteArray, QByteArray> map;
    int startIndex = 0;
    int pos = 0;
    while ((pos = ba.indexOf("=", startIndex)) >= 0)
    {
        // key get name and skip equals
        const QByteArray key = ba.mid(startIndex, pos - startIndex).trimmed();
        pos++;

        // check whether string is quoted
        if (ba.at(pos) == '"')
        {
            // skip opening quote
            pos++;
            int endPos = ba.indexOf('"', pos);
            // skip quoted quotes
            while (endPos >= 0 && ba.at(endPos - 1) == '\\')
                endPos = ba.indexOf('"', endPos + 1);
            if (endPos < 0)
            {
                qWarning("Unfinished quoted string");
                return map;
            }
            // unquote
            QByteArray value = ba.mid(pos, endPos - pos);
            value.replace("\\\"", "\"");
            value.replace("\\\\", "\\");
            map[key] = value;
            // skip closing quote and comma
            startIndex = endPos + 2;
        } else {
            // non-quoted string
            int endPos = ba.indexOf(',', pos);
            if (endPos < 0)
                endPos = ba.size();
            map[key] = ba.mid(pos, endPos - pos);
            // skip comma
            startIndex = endPos + 1;
        }
    }
    return map;
}

QByteArray QXmppSaslDigestMd5::serializeMessage(const QMap<QByteArray, QByteArray> &map)
{
    QByteArray ba;
    foreach (const QByteArray &key, map.keys())
    {
        if (!ba.isEmpty())
            ba.append(',');
        ba.append(key + "=");
        QByteArray value = map[key];
        const char *separators = "()<>@,;:\\\"/[]?={} \t";
        bool quote = false;
        for (const char *c = separators; *c; c++)
        {
            if (value.contains(*c))
            {
                quote = true;
                break;
            }
        }
        if (quote)
        {
            value.replace("\\", "\\\\");
            value.replace("\"", "\\\"");
            ba.append("\"" + value + "\"");
        }
        else
            ba.append(value);
    }
    return ba;
}

