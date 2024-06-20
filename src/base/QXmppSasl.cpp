// SPDX-FileCopyrightText: 2012 Manjeet Dahiya <manjeetdahiya@gmail.com>
// SPDX-FileCopyrightText: 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppConstants_p.h"
#include "QXmppSasl2UserAgent.h"
#include "QXmppSasl_p.h"
#include "QXmppUtils.h"
#include "QXmppUtils_p.h"

#include "StringLiterals.h"

#include <QDomElement>
#include <QMessageAuthenticationCode>
#include <QUrlQuery>
#include <QXmlStreamWriter>
#include <QtEndian>

using namespace QXmpp::Private;

static QByteArray forcedNonce;

constexpr auto SASL_ERROR_CONDITIONS = to_array<QStringView>({
    u"aborted",
    u"account-disabled",
    u"credentials-expired",
    u"encryption-required",
    u"incorrect-encoding",
    u"invalid-authzid",
    u"invalid-mechanism",
    u"malformed-request",
    u"mechanism-too-weak",
    u"not-authorized",
    u"temporary-auth-failure",
});

namespace QXmpp::Private {

namespace Sasl {

QString errorConditionToString(ErrorCondition c)
{
    return SASL_ERROR_CONDITIONS.at(size_t(c)).toString();
}

std::optional<ErrorCondition> errorConditionFromString(QStringView str)
{
    return enumFromString<ErrorCondition>(SASL_ERROR_CONDITIONS, str);
}

std::optional<Auth> Auth::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"auth" || el.namespaceURI() != ns_sasl) {
        return {};
    }

    Auth auth;
    if (auto value = parseBase64(el.text())) {
        auth.value = *value;
    } else {
        return {};
    }
    auth.mechanism = el.attribute(u"mechanism"_s);
    return auth;
}

void Auth::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("auth"));
    writer->writeDefaultNamespace(toString65(ns_sasl));
    writer->writeAttribute(QSL65("mechanism"), mechanism);
    if (!value.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        writer->writeCharacters(value.toBase64());
#else
        writer->writeCharacters(serializeBase64(value));
#endif
    }
    writer->writeEndElement();
}

std::optional<Challenge> Challenge::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"challenge" || el.namespaceURI() != ns_sasl) {
        return {};
    }

    if (auto value = parseBase64(el.text())) {
        return Challenge { *value };
    }
    return {};
}

void Challenge::toXml(QXmlStreamWriter *writer) const
{
    writeXmlTextElement(writer, u"challenge", ns_sasl, serializeBase64(value));
}

std::optional<Failure> Failure::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"failure" || el.namespaceURI() != ns_sasl) {
        return {};
    }

    auto errorConditionString = el.firstChildElement().tagName();

    Failure failure {
        errorConditionFromString(errorConditionString),
        el.firstChildElement(u"text"_s).text(),
    };

    // RFC3920 defines the error condition as "not-authorized", but
    // some broken servers use "bad-auth" instead. We tolerate this
    // by remapping the error to "not-authorized".
    if (!failure.condition && errorConditionString == u"bad-auth") {
        failure.condition = ErrorCondition::NotAuthorized;
    }

    return failure;
}

void Failure::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("failure"));
    writer->writeDefaultNamespace(toString65(ns_sasl));
    if (condition) {
        writer->writeEmptyElement(toString65(SASL_ERROR_CONDITIONS.at(size_t(*condition))));
    }

    if (!text.isEmpty()) {
        writer->writeStartElement(u"text"_s);
        writer->writeAttribute(u"xml:lang"_s, u"en"_s);
        writer->writeCharacters(text);
        writer->writeEndElement();
    }

    writer->writeEndElement();
}

std::optional<Response> Response::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"response" || el.namespaceURI() != ns_sasl) {
        return {};
    }

    if (auto value = parseBase64(el.text())) {
        return Response { *value };
    }
    return {};
}

void Response::toXml(QXmlStreamWriter *writer) const
{
    writeXmlTextElement(writer, u"response", ns_sasl, serializeBase64(value));
}

std::optional<Success> Success::fromDom(const QDomElement &el)
{
    if (el.tagName() == u"success" && el.namespaceURI() == ns_sasl) {
        return Success();
    }
    return {};
}

void Success::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("success"));
    writer->writeDefaultNamespace(toString65(ns_sasl));
    writer->writeEndElement();
}

}  // namespace Sasl

std::optional<Bind2Feature> Bind2Feature::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"bind" || el.namespaceURI() != ns_bind2) {
        return {};
    }

    Bind2Feature bind2;

    auto inlineElement = firstChildElement(el, u"inline", ns_bind2);
    for (const auto &featureEl : iterChildElements(inlineElement, u"feature", ns_bind2)) {
        bind2.features.push_back(featureEl.attribute(u"var"_s));
    }

    return bind2;
}

void Bind2Feature::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("bind"));
    writer->writeDefaultNamespace(toString65(ns_bind2));
    if (!features.empty()) {
        writer->writeStartElement(QSL65("inline"));
        for (const auto &feature : features) {
            writer->writeStartElement(QSL65("feature"));
            writer->writeAttribute(QSL65("var"), feature);
            writer->writeEndElement();
        }
        writer->writeEndElement();
    }
    writer->writeEndElement();
}

std::optional<Bind2Request> Bind2Request::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"bind" || el.namespaceURI() != ns_bind2) {
        return {};
    }

    return Bind2Request {
        firstChildElement(el, u"tag", ns_bind2).text(),
        !firstChildElement(el, u"inactive", ns_csi).isNull(),
        !firstChildElement(el, u"enable", ns_carbons).isNull(),
        SmEnable::fromDom(firstChildElement(el, u"enable", ns_stream_management)),
    };
}

void Bind2Request::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("bind"));
    writer->writeDefaultNamespace(toString65(ns_bind2));
    writeOptionalXmlTextElement(writer, u"tag", tag);
    if (csiInactive) {
        writeEmptyElement(writer, u"inactive", ns_csi);
    }
    if (carbonsEnable) {
        writeEmptyElement(writer, u"enable", ns_carbons);
    }
    if (smEnable) {
        smEnable->toXml(writer);
    }
    writer->writeEndElement();
}

std::optional<Bind2Bound> Bind2Bound::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"bound" || el.namespaceURI() != ns_bind2) {
        return {};
    }

    return Bind2Bound {
        .smFailed = SmFailed::fromDom(firstChildElement(el, u"failed", ns_stream_management)),
        .smEnabled = SmEnabled::fromDom(firstChildElement(el, u"enabled", ns_stream_management)),
    };
}

void Bind2Bound::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("bound"));
    writer->writeDefaultNamespace(toString65(ns_bind2));
    if (smFailed) {
        smFailed->toXml(writer);
    }
    if (smEnabled) {
        smEnabled->toXml(writer);
    }
    writer->writeEndElement();
}

namespace Sasl2 {

std::optional<StreamFeature> StreamFeature::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"authentication" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    StreamFeature feature;

    for (const auto &mechEl : iterChildElements(el, u"mechanism", ns_sasl_2)) {
        feature.mechanisms.push_back(mechEl.text());
    }

    if (auto inlineEl = firstChildElement(el, u"inline", ns_sasl_2); !inlineEl.isNull()) {
        feature.bind2Feature = Bind2Feature::fromDom(firstChildElement(inlineEl, u"bind", ns_bind2));
        feature.streamResumptionAvailable = !firstChildElement(inlineEl, u"sm", ns_stream_management).isNull();
    }
    return feature;
}

void StreamFeature::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("authentication"));
    writer->writeDefaultNamespace(toString65(ns_sasl_2));
    for (const auto &mechanism : mechanisms) {
        writeXmlTextElement(writer, u"mechanism", mechanism);
    }
    if (bind2Feature || streamResumptionAvailable) {
        writer->writeStartElement(QSL65("inline"));
        if (bind2Feature) {
            bind2Feature->toXml(writer);
        }
        if (streamResumptionAvailable) {
            writeEmptyElement(writer, u"sm", ns_stream_management);
        }
        writer->writeEndElement();
    }
    writer->writeEndElement();
}

std::optional<UserAgent> UserAgent::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"user-agent" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    return UserAgent {
        QUuid::fromString(el.attribute(u"id"_s)),
        firstChildElement(el, u"software", ns_sasl_2).text(),
        firstChildElement(el, u"device", ns_sasl_2).text(),
    };
}

void UserAgent::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("user-agent"));
    if (!id.isNull()) {
        writer->writeAttribute(QSL65("id"), id.toString(QUuid::WithoutBraces));
    }
    writeOptionalXmlTextElement(writer, u"software", software);
    writeOptionalXmlTextElement(writer, u"device", device);
    writer->writeEndElement();
}

std::optional<Authenticate> Authenticate::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"authenticate" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }
    return Authenticate {
        el.attribute(u"mechanism"_s),
        parseBase64(firstChildElement(el, u"initial-response", ns_sasl_2).text()).value_or(QByteArray()),
        UserAgent::fromDom(firstChildElement(el, u"user-agent", ns_sasl_2)),
        Bind2Request::fromDom(firstChildElement(el, u"bind", ns_bind2)),
        SmResume::fromDom(firstChildElement(el, u"resume", ns_stream_management)),
    };
}

void Authenticate::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("authenticate"));
    writer->writeDefaultNamespace(toString65(ns_sasl_2));
    writer->writeAttribute(QSL65("mechanism"), mechanism);
    writeOptionalXmlTextElement(writer, u"initial-response", QString::fromUtf8(initialResponse.toBase64()));
    if (userAgent) {
        userAgent->toXml(writer);
    }
    if (bindRequest) {
        bindRequest->toXml(writer);
    }
    if (smResume) {
        smResume->toXml(writer);
    }
    writer->writeEndElement();
}

std::optional<Challenge> Challenge::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"challenge" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    if (auto result = parseBase64(el.text())) {
        return Challenge { *result };
    }
    return {};
}

void Challenge::toXml(QXmlStreamWriter *writer) const
{
    writeXmlTextElement(writer, u"challenge", ns_sasl_2, serializeBase64(data));
}

std::optional<Response> Response::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"response" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    if (auto result = parseBase64(el.text())) {
        return Response { *result };
    }
    return {};
}

void Response::toXml(QXmlStreamWriter *writer) const
{
    writeXmlTextElement(writer, u"response", ns_sasl_2, serializeBase64(data));
}

std::optional<Success> Success::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"success" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    Success output;

    if (auto subEl = firstChildElement(el, u"additional-data", ns_sasl_2); !subEl.isNull()) {
        if (auto result = parseBase64(subEl.text())) {
            output.additionalData = *result;
        } else {
            // invalid base64 data
            return {};
        }
    }

    output.authorizationIdentifier = firstChildElement(el, u"authorization-identifier", ns_sasl_2).text();
    output.bound = Bind2Bound::fromDom(firstChildElement(el, u"bound", ns_bind2));
    output.smResumed = SmResumed::fromDom(firstChildElement(el, u"resumed", ns_stream_management));
    output.smFailed = SmFailed::fromDom(firstChildElement(el, u"failed", ns_stream_management));

    return output;
}

void Success::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("success"));
    writer->writeDefaultNamespace(toString65(ns_sasl_2));
    if (additionalData) {
        writer->writeTextElement(QSL65("additional-data"), serializeBase64(*additionalData));
    }
    writeXmlTextElement(writer, u"authorization-identifier", authorizationIdentifier);
    if (bound) {
        bound->toXml(writer);
    }
    if (smResumed) {
        smResumed->toXml(writer);
    }
    if (smFailed) {
        smFailed->toXml(writer);
    }
    writer->writeEndElement();
}

std::optional<Failure> Failure::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"failure" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    // SASL error condition
    auto condition = Sasl::errorConditionFromString(firstChildElement(el, {}, ns_sasl).tagName());
    if (!condition) {
        return {};
    }

    return Failure {
        condition.value(),
        firstChildElement(el, u"text", ns_sasl_2).text(),
    };
}

void Failure::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("failure"));
    writer->writeDefaultNamespace(toString65(ns_sasl_2));
    writeEmptyElement(writer, Sasl::errorConditionToString(condition), ns_sasl);
    writeOptionalXmlTextElement(writer, u"text", text);
    writer->writeEndElement();
}

std::optional<Continue> Continue::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"continue" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }

    Continue output;

    if (auto subEl = firstChildElement(el, u"additional-data", ns_sasl_2); !subEl.isNull()) {
        if (auto result = parseBase64(subEl.text())) {
            output.additionalData = *result;
        } else {
            // invalid base64 data
            return {};
        }
    }

    for (const auto &taskEl : iterChildElements(firstChildElement(el, u"tasks", ns_sasl_2))) {
        output.tasks.push_back(taskEl.text());
    }
    // tasks are mandatory
    if (output.tasks.empty()) {
        return {};
    }

    output.text = firstChildElement(el, u"text", ns_sasl_2).text();

    return output;
}

void Continue::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("continue"));
    writer->writeDefaultNamespace(toString65(ns_sasl_2));
    writeOptionalXmlTextElement(writer, u"additional-data", serializeBase64(additionalData));
    writer->writeStartElement(QSL65("tasks"));
    for (const auto &task : tasks) {
        writer->writeTextElement(QSL65("task"), task);
    }
    writer->writeEndElement();
    writeOptionalXmlTextElement(writer, u"text", text);
    writer->writeEndElement();
}

std::optional<Abort> Abort::fromDom(const QDomElement &el)
{
    if (el.tagName() != u"abort" || el.namespaceURI() != ns_sasl_2) {
        return {};
    }
    return Abort { firstChildElement(el, u"text", ns_sasl_2).text() };
}

void Abort::toXml(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QSL65("abort"));
    writer->writeDefaultNamespace(toString65(ns_sasl_2));
    writeOptionalXmlTextElement(writer, u"text", text);
    writer->writeEndElement();
}

}  // namespace Sasl2

}  // namespace QXmpp::Private

///
/// \class QXmppSasl2UserAgent
///
/// \brief User-agent for identifying devices across reconnects, defined in \xep{0388, Extensible
/// SASL Profile}.
///
/// \since QXmpp 1.7
///

struct QXmppSasl2UserAgentPrivate : QSharedData, Sasl2::UserAgent { };

/// Default-constructor
QXmppSasl2UserAgent::QXmppSasl2UserAgent()
    : d(new QXmppSasl2UserAgentPrivate())
{
}

/// Constructs a new user-agent with given values.
QXmppSasl2UserAgent::QXmppSasl2UserAgent(QUuid deviceId, const QString &softwareName, const QString &deviceName)
    : d(new QXmppSasl2UserAgentPrivate { QSharedData(), Sasl2::UserAgent { deviceId, softwareName, deviceName } })
{
}

QXMPP_PRIVATE_DEFINE_RULE_OF_SIX(QXmppSasl2UserAgent)

///
/// Returns the unique and stable ID of this device.
///
/// This ID is intended to be persistent across reconnects and reboots of the used device.
///
QUuid QXmppSasl2UserAgent::deviceId() const
{
    return d->id;
}

///
/// Sets the unique and stable ID of this device.
///
/// This ID is intended to be persistent across reconnects and reboots of the used device.
///
void QXmppSasl2UserAgent::setDeviceId(QUuid id)
{
    d->id = id;
}

///
/// Returns the name of the used software (e.g. *AwesomeXMPP*).
///
const QString &QXmppSasl2UserAgent::softwareName() const
{
    return d->software;
}

///
/// Sets the name of the used software (e.g. *AwesomeXMPP*).
///
void QXmppSasl2UserAgent::setSoftwareName(const QString &software)
{
    d->software = software;
}

///
/// Returns the name of this device (e.g. *Kiva's Phone*).
///
const QString &QXmppSasl2UserAgent::deviceName() const
{
    return d->device;
}

///
/// Sets the name of this device (e.g. *Kiva's Phone*).
///
void QXmppSasl2UserAgent::setDeviceName(const QString &device)
{
    d->device = device;
}

// When adding new algorithms, also add them to QXmppSaslClient::availableMechanisms().
static const QMap<QStringView, QCryptographicHash::Algorithm> SCRAM_ALGORITHMS = {
    { u"SCRAM-SHA-1", QCryptographicHash::Sha1 },
    { u"SCRAM-SHA-256", QCryptographicHash::Sha256 },
    { u"SCRAM-SHA-512", QCryptographicHash::Sha512 },
    { u"SCRAM-SHA3-512", QCryptographicHash::RealSha3_512 },
};

// Calculate digest response for use with XMPP/SASL.

static QByteArray calculateDigest(const QByteArray &method, const QByteArray &digestUri, const QByteArray &secret, const QByteArray &nonce, const QByteArray &cnonce, const QByteArray &nc)
{
    const QByteArray A1 = secret + ':' + nonce + ':' + cnonce;
    const QByteArray A2 = method + ':' + digestUri;

    QByteArray HA1 = QCryptographicHash::hash(A1, QCryptographicHash::Md5).toHex();
    QByteArray HA2 = QCryptographicHash::hash(A2, QCryptographicHash::Md5).toHex();
    const QByteArray KD = HA1 + ':' + nonce + ':' + nc + ':' + cnonce + ":auth:" + HA2;
    return QCryptographicHash::hash(KD, QCryptographicHash::Md5).toHex();
}

// Perform PBKFD2 key derivation, code taken from Qt 5.12

static QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                  const QByteArray &data, const QByteArray &salt,
                                  int iterations, uint32_t dkLen)
{
    QByteArray key;
    quint32 currentIteration = 1;
    QMessageAuthenticationCode hmac(algorithm, data);
    QByteArray index(4, Qt::Uninitialized);
    while (key.length() < dkLen) {
        hmac.addData(salt);
        qToBigEndian(currentIteration, reinterpret_cast<uchar *>(index.data()));
        hmac.addData(index);
        QByteArray u = hmac.result();
        hmac.reset();
        QByteArray tkey = u;
        for (int iter = 1; iter < iterations; iter++) {
            hmac.addData(u);
            u = hmac.result();
            hmac.reset();
            std::transform(tkey.cbegin(), tkey.cend(), u.cbegin(), tkey.begin(),
                           std::bit_xor<char>());
        }
        key += tkey;
        currentIteration++;
    }
    return key.left(dkLen);
}

static QByteArray generateNonce()
{
    if (!forcedNonce.isEmpty()) {
        return forcedNonce;
    }

    QByteArray nonce = QXmppUtils::generateRandomBytes(32);

    // The random data can the '=' char is not valid as it is a delimiter,
    // so to be safe, base64 the nonce
    return nonce.toBase64();
}

static QMap<char, QByteArray> parseGS2(const QByteArray &ba)
{
    QMap<char, QByteArray> map;
    const auto keyValuePairs = ba.split(u',');
    for (const auto &keyValue : keyValuePairs) {
        if (keyValue.size() >= 2 && keyValue[1] == '=') {
            map[keyValue[0]] = keyValue.mid(2);
        }
    }
    return map;
}

///
/// Returns a list of supported mechanisms.
///
QStringList QXmppSaslClient::availableMechanisms()
{
    return {
        u"SCRAM-SHA3-512"_s,
        u"SCRAM-SHA-512"_s,
        u"SCRAM-SHA-256"_s,
        u"SCRAM-SHA-1"_s,
        u"DIGEST-MD5"_s,
        u"PLAIN"_s,
        u"ANONYMOUS"_s,
        u"X-FACEBOOK-PLATFORM"_s,
        u"X-MESSENGER-OAUTH2"_s,
        u"X-OAUTH2"_s,
    };
}

///
/// Creates an SASL client for the given mechanism.
///
std::unique_ptr<QXmppSaslClient> QXmppSaslClient::create(const QString &mechanism, QObject *parent)
{
    if (mechanism == u"PLAIN") {
        return std::make_unique<QXmppSaslClientPlain>(parent);
    } else if (mechanism == u"DIGEST-MD5") {
        return std::make_unique<QXmppSaslClientDigestMd5>(parent);
    } else if (mechanism == u"ANONYMOUS") {
        return std::make_unique<QXmppSaslClientAnonymous>(parent);
    } else if (SCRAM_ALGORITHMS.contains(mechanism)) {
        return std::make_unique<QXmppSaslClientScram>(SCRAM_ALGORITHMS.value(mechanism), parent);
    } else if (mechanism == u"X-FACEBOOK-PLATFORM") {
        return std::make_unique<QXmppSaslClientFacebook>(parent);
    } else if (mechanism == u"X-MESSENGER-OAUTH2") {
        return std::make_unique<QXmppSaslClientWindowsLive>(parent);
    } else if (mechanism == u"X-OAUTH2") {
        return std::make_unique<QXmppSaslClientGoogle>(parent);
    } else {
        return nullptr;
    }
}

QXmppSaslClientAnonymous::QXmppSaslClientAnonymous(QObject *parent)
    : QXmppSaslClient(parent), m_step(0)
{
}

QString QXmppSaslClientAnonymous::mechanism() const
{
    return u"ANONYMOUS"_s;
}

std::optional<QByteArray> QXmppSaslClientAnonymous::respond(const QByteArray &)
{
    if (m_step == 0) {
        m_step++;
        return QByteArray();
    } else {
        warning(u"QXmppSaslClientAnonymous : Invalid step"_s);
        return {};
    }
}

QXmppSaslClientDigestMd5::QXmppSaslClientDigestMd5(QObject *parent)
    : QXmppSaslClient(parent), m_nc(QByteArrayLiteral("00000001")), m_step(0)
{
    m_cnonce = generateNonce();
}

void QXmppSaslClientDigestMd5::setCredentials(const QXmpp::Private::Credentials &credentials)
{
    m_password = credentials.password;
}

QString QXmppSaslClientDigestMd5::mechanism() const
{
    return u"DIGEST-MD5"_s;
}

std::optional<QByteArray> QXmppSaslClientDigestMd5::respond(const QByteArray &challenge)
{
    const QByteArray digestUri = u"%1/%2"_s.arg(serviceType(), host()).toUtf8();

    if (m_step == 0) {
        m_step++;
        return QByteArray();
    } else if (m_step == 1) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(challenge);

        if (!input.contains(QByteArrayLiteral("nonce"))) {
            warning(u"QXmppSaslClientDigestMd5 : Invalid input on step 1"_s);
            return {};
        }

        // determine realm
        const QByteArray realm = input.value(QByteArrayLiteral("realm"));

        // determine quality of protection
        const QList<QByteArray> qops = input.value(QByteArrayLiteral("qop"), QByteArrayLiteral("auth")).split(',');
        if (!qops.contains(QByteArrayLiteral("auth"))) {
            warning(u"QXmppSaslClientDigestMd5 : Invalid quality of protection"_s);
            return {};
        }

        m_nonce = input.value(QByteArrayLiteral("nonce"));
        m_secret = QCryptographicHash::hash(
            QByteArray(username().toUtf8() + QByteArrayLiteral(":") + realm + QByteArrayLiteral(":") + m_password.toUtf8()),
            QCryptographicHash::Md5);

        // Build response
        QMap<QByteArray, QByteArray> output;
        output[QByteArrayLiteral("username")] = username().toUtf8();
        if (!realm.isEmpty()) {
            output[QByteArrayLiteral("realm")] = realm;
        }
        output[QByteArrayLiteral("nonce")] = m_nonce;
        output[QByteArrayLiteral("qop")] = QByteArrayLiteral("auth");
        output[QByteArrayLiteral("cnonce")] = m_cnonce;
        output[QByteArrayLiteral("nc")] = m_nc;
        output[QByteArrayLiteral("digest-uri")] = digestUri;
        output[QByteArrayLiteral("response")] = calculateDigest(QByteArrayLiteral("AUTHENTICATE"), digestUri, m_secret, m_nonce, m_cnonce, m_nc);
        output[QByteArrayLiteral("charset")] = QByteArrayLiteral("utf-8");

        m_step++;
        return QXmppSaslDigestMd5::serializeMessage(output);
    } else if (m_step == 2) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(challenge);

        // check new challenge
        if (input.value(QByteArrayLiteral("rspauth")) != calculateDigest(QByteArray(), digestUri, m_secret, m_nonce, m_cnonce, m_nc)) {
            warning(u"QXmppSaslClientDigestMd5 : Invalid challenge on step 2"_s);
            return {};
        }

        m_step++;
        return QByteArray();
    } else {
        warning(u"QXmppSaslClientDigestMd5 : Invalid step"_s);
        return {};
    }
}

QXmppSaslClientFacebook::QXmppSaslClientFacebook(QObject *parent)
    : QXmppSaslClient(parent), m_step(0)
{
}

void QXmppSaslClientFacebook::setCredentials(const QXmpp::Private::Credentials &credentials)
{
    m_accessToken = credentials.facebookAccessToken;
    m_appId = credentials.facebookAppId;
}

QString QXmppSaslClientFacebook::mechanism() const
{
    return u"X-FACEBOOK-PLATFORM"_s;
}

std::optional<QByteArray> QXmppSaslClientFacebook::respond(const QByteArray &challenge)
{
    if (m_step == 0) {
        // no initial response
        m_step++;
        return QByteArray();
    } else if (m_step == 1) {
        // parse request
        QUrlQuery requestUrl(QString::fromUtf8(challenge));
        if (!requestUrl.hasQueryItem(u"method"_s) || !requestUrl.hasQueryItem(u"nonce"_s)) {
            warning(u"QXmppSaslClientFacebook : Invalid challenge, nonce or method missing"_s);
            return {};
        }

        // build response
        QUrlQuery responseUrl;
        responseUrl.addQueryItem(u"access_token"_s, m_accessToken);
        responseUrl.addQueryItem(u"api_key"_s, m_appId);
        responseUrl.addQueryItem(u"call_id"_s, QString());
        responseUrl.addQueryItem(u"method"_s, requestUrl.queryItemValue(u"method"_s));
        responseUrl.addQueryItem(u"nonce"_s, requestUrl.queryItemValue(u"nonce"_s));
        responseUrl.addQueryItem(u"v"_s, u"1.0"_s);

        m_step++;
        return responseUrl.query().toUtf8();
    } else {
        warning(u"QXmppSaslClientFacebook : Invalid step"_s);
        return {};
    }
}

QXmppSaslClientGoogle::QXmppSaslClientGoogle(QObject *parent)
    : QXmppSaslClient(parent), m_step(0)
{
}

void QXmppSaslClientGoogle::setCredentials(const QXmpp::Private::Credentials &credentials)
{
    m_accessToken = credentials.googleAccessToken;
}

QString QXmppSaslClientGoogle::mechanism() const
{
    return u"X-OAUTH2"_s;
}

std::optional<QByteArray> QXmppSaslClientGoogle::respond(const QByteArray &)
{
    if (m_step == 0) {
        // send initial response
        m_step++;
        return QString(u'\0' + username() + u'\0' + m_accessToken).toUtf8();
    } else {
        warning(u"QXmppSaslClientGoogle : Invalid step"_s);
        return {};
    }
}

QXmppSaslClientPlain::QXmppSaslClientPlain(QObject *parent)
    : QXmppSaslClient(parent), m_step(0)
{
}

void QXmppSaslClientPlain::setCredentials(const QXmpp::Private::Credentials &credentials)
{
    m_password = credentials.password;
}

QString QXmppSaslClientPlain::mechanism() const
{
    return u"PLAIN"_s;
}

std::optional<QByteArray> QXmppSaslClientPlain::respond(const QByteArray &)
{
    if (m_step == 0) {
        m_step++;
        return QString(u'\0' + username() + u'\0' + m_password).toUtf8();
    } else {
        warning(u"QXmppSaslClientPlain : Invalid step"_s);
        return {};
    }
}

QXmppSaslClientScram::QXmppSaslClientScram(QCryptographicHash::Algorithm algorithm, QObject *parent)
    : QXmppSaslClient(parent),
      m_algorithm(algorithm),
      m_step(0),
      m_dklen(QCryptographicHash::hashLength(algorithm))
{
    const auto itr = std::find(SCRAM_ALGORITHMS.cbegin(), SCRAM_ALGORITHMS.cend(), algorithm);
    Q_ASSERT(itr != SCRAM_ALGORITHMS.cend());

    m_nonce = generateNonce();
}

void QXmppSaslClientScram::setCredentials(const QXmpp::Private::Credentials &credentials)
{
    m_password = credentials.password;
}

QString QXmppSaslClientScram::mechanism() const
{
    return SCRAM_ALGORITHMS.key(m_algorithm).toString();
}

std::optional<QByteArray> QXmppSaslClientScram::respond(const QByteArray &challenge)
{
    if (m_step == 0) {
        m_gs2Header = QByteArrayLiteral("n,,");
        m_clientFirstMessageBare = QByteArrayLiteral("n=") + username().toUtf8() + QByteArrayLiteral(",r=") + m_nonce;

        m_step++;
        return m_gs2Header + m_clientFirstMessageBare;
    } else if (m_step == 1) {
        // validate input
        const QMap<char, QByteArray> input = parseGS2(challenge);
        const QByteArray nonce = input.value('r');
        const QByteArray salt = QByteArray::fromBase64(input.value('s'));
        const int iterations = input.value('i').toInt();
        if (!nonce.startsWith(m_nonce) || salt.isEmpty() || iterations < 1) {
            return {};
        }

        // calculate proofs
        const QByteArray clientFinalMessageBare = QByteArrayLiteral("c=") + m_gs2Header.toBase64() + QByteArrayLiteral(",r=") + nonce;
        const QByteArray saltedPassword = deriveKeyPbkdf2(m_algorithm, m_password.toUtf8(), salt,
                                                          iterations, m_dklen);
        const QByteArray clientKey = QMessageAuthenticationCode::hash(QByteArrayLiteral("Client Key"), saltedPassword, m_algorithm);
        const QByteArray storedKey = QCryptographicHash::hash(clientKey, m_algorithm);
        const QByteArray authMessage = m_clientFirstMessageBare + QByteArrayLiteral(",") + challenge + QByteArrayLiteral(",") + clientFinalMessageBare;
        QByteArray clientProof = QMessageAuthenticationCode::hash(authMessage, storedKey, m_algorithm);
        std::transform(clientProof.cbegin(), clientProof.cend(), clientKey.cbegin(),
                       clientProof.begin(), std::bit_xor<char>());

        const QByteArray serverKey = QMessageAuthenticationCode::hash(QByteArrayLiteral("Server Key"), saltedPassword, m_algorithm);
        m_serverSignature = QMessageAuthenticationCode::hash(authMessage, serverKey, m_algorithm);

        m_step++;
        return clientFinalMessageBare + QByteArrayLiteral(",p=") + clientProof.toBase64();
    } else if (m_step == 2) {
        const QMap<char, QByteArray> input = parseGS2(challenge);
        m_step++;
        if (QByteArray::fromBase64(input.value('v')) == m_serverSignature) {
            return QByteArray();
        }
        return {};
    } else {
        warning(u"QXmppSaslClientPlain : Invalid step"_s);
        return {};
    }
}

QXmppSaslClientWindowsLive::QXmppSaslClientWindowsLive(QObject *parent)
    : QXmppSaslClient(parent), m_step(0)
{
}

void QXmppSaslClientWindowsLive::setCredentials(const QXmpp::Private::Credentials &credentials)
{
    m_accessToken = credentials.windowsLiveAccessToken;
}

QString QXmppSaslClientWindowsLive::mechanism() const
{
    return u"X-MESSENGER-OAUTH2"_s;
}

std::optional<QByteArray> QXmppSaslClientWindowsLive::respond(const QByteArray &)
{
    if (m_step == 0) {
        // send initial response
        m_step++;
        return QByteArray::fromBase64(m_accessToken.toLatin1());
    } else {
        warning(u"QXmppSaslClientWindowsLive : Invalid step"_s);
        return {};
    }
}

class QXmppSaslServerPrivate
{
public:
    QString username;
    QString password;
    QByteArray passwordDigest;
    QString realm;
};

QXmppSaslServer::QXmppSaslServer(QObject *parent)
    : QXmppLoggable(parent),
      d(std::make_unique<QXmppSaslServerPrivate>())
{
}

QXmppSaslServer::~QXmppSaslServer() = default;

/// Creates an SASL server for the given mechanism.
std::unique_ptr<QXmppSaslServer> QXmppSaslServer::create(const QString &mechanism, QObject *parent)
{
    if (mechanism == u"PLAIN") {
        return std::make_unique<QXmppSaslServerPlain>(parent);
    } else if (mechanism == u"DIGEST-MD5") {
        return std::make_unique<QXmppSaslServerDigestMd5>(parent);
    } else if (mechanism == u"ANONYMOUS") {
        return std::make_unique<QXmppSaslServerAnonymous>(parent);
    } else {
        return {};
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

/// Returns the password digest.
QByteArray QXmppSaslServer::passwordDigest() const
{
    return d->passwordDigest;
}

/// Sets the password digest.
void QXmppSaslServer::setPasswordDigest(const QByteArray &digest)
{
    d->passwordDigest = digest;
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
    : QXmppSaslServer(parent), m_step(0)
{
}

QString QXmppSaslServerAnonymous::mechanism() const
{
    return u"ANONYMOUS"_s;
}

QXmppSaslServer::Response QXmppSaslServerAnonymous::respond(const QByteArray &request, QByteArray &response)
{
    Q_UNUSED(request);
    if (m_step == 0) {
        m_step++;
        response = QByteArray();
        return Succeeded;
    } else {
        warning(u"QXmppSaslServerAnonymous : Invalid step"_s);
        return Failed;
    }
}

QXmppSaslServerDigestMd5::QXmppSaslServerDigestMd5(QObject *parent)
    : QXmppSaslServer(parent), m_step(0)
{
    m_nonce = generateNonce();
}

QString QXmppSaslServerDigestMd5::mechanism() const
{
    return u"DIGEST-MD5"_s;
}

QXmppSaslServer::Response QXmppSaslServerDigestMd5::respond(const QByteArray &request, QByteArray &response)
{
    if (m_step == 0) {
        QMap<QByteArray, QByteArray> output;
        output[QByteArrayLiteral("nonce")] = m_nonce;
        if (!realm().isEmpty()) {
            output[QByteArrayLiteral("realm")] = realm().toUtf8();
        }
        output[QByteArrayLiteral("qop")] = QByteArrayLiteral("auth");
        output[QByteArrayLiteral("charset")] = QByteArrayLiteral("utf-8");
        output[QByteArrayLiteral("algorithm")] = QByteArrayLiteral("md5-sess");

        m_step++;
        response = QXmppSaslDigestMd5::serializeMessage(output);
        return Challenge;
    } else if (m_step == 1) {
        const QMap<QByteArray, QByteArray> input = QXmppSaslDigestMd5::parseMessage(request);
        const QByteArray realm = input.value(QByteArrayLiteral("realm"));
        const QByteArray digestUri = input.value(QByteArrayLiteral("digest-uri"));

        if (input.value(QByteArrayLiteral("qop")) != QByteArrayLiteral("auth")) {
            warning(u"QXmppSaslServerDigestMd5 : Invalid quality of protection"_s);
            return Failed;
        }

        setUsername(QString::fromUtf8(input.value(QByteArrayLiteral("username"))));
        if (password().isEmpty() && passwordDigest().isEmpty()) {
            return InputNeeded;
        }

        m_nc = input.value(QByteArrayLiteral("nc"));
        m_cnonce = input.value(QByteArrayLiteral("cnonce"));
        if (!password().isEmpty()) {
            m_secret = QCryptographicHash::hash(
                QByteArray(username().toUtf8() + QByteArrayLiteral(":") + realm + QByteArrayLiteral(":") + password().toUtf8()),
                QCryptographicHash::Md5);
        } else {
            m_secret = passwordDigest();
        }

        if (input.value(QByteArrayLiteral("response")) != calculateDigest(QByteArrayLiteral("AUTHENTICATE"), digestUri, m_secret, m_nonce, m_cnonce, m_nc)) {
            return Failed;
        }

        QMap<QByteArray, QByteArray> output;
        output[QByteArrayLiteral("rspauth")] = calculateDigest(QByteArray(), digestUri, m_secret, m_nonce, m_cnonce, m_nc);

        m_step++;
        response = QXmppSaslDigestMd5::serializeMessage(output);
        return Challenge;
    } else if (m_step == 2) {
        m_step++;
        response = QByteArray();
        return Succeeded;
    } else {
        warning(u"QXmppSaslServerDigestMd5 : Invalid step"_s);
        return Failed;
    }
}

QXmppSaslServerPlain::QXmppSaslServerPlain(QObject *parent)
    : QXmppSaslServer(parent), m_step(0)
{
}

QString QXmppSaslServerPlain::mechanism() const
{
    return u"PLAIN"_s;
}

QXmppSaslServer::Response QXmppSaslServerPlain::respond(const QByteArray &request, QByteArray &response)
{
    if (m_step == 0) {
        if (request.isEmpty()) {
            response = QByteArray();
            return Challenge;
        }

        QList<QByteArray> auth = request.split('\0');
        if (auth.size() != 3) {
            warning(u"QXmppSaslServerPlain : Invalid input"_s);
            return Failed;
        }
        setUsername(QString::fromUtf8(auth[1]));
        setPassword(QString::fromUtf8(auth[2]));

        m_step++;
        response = QByteArray();
        return InputNeeded;
    } else {
        warning(u"QXmppSaslServerPlain : Invalid step"_s);
        return Failed;
    }
}

void QXmppSaslDigestMd5::setNonce(const QByteArray &nonce)
{
    forcedNonce = nonce;
}

QMap<QByteArray, QByteArray> QXmppSaslDigestMd5::parseMessage(const QByteArray &ba)
{
    QMap<QByteArray, QByteArray> map;
    int startIndex = 0;
    int pos = 0;
    while ((pos = ba.indexOf('=', startIndex)) >= 0) {
        // key get name and skip equals
        const QByteArray key = ba.mid(startIndex, pos - startIndex).trimmed();
        pos++;

        if (pos == ba.size()) {
            // end of the input
            map.insert(key, QByteArray());
            startIndex = pos;
        } else if (ba.at(pos) == '"') {
            // check whether string is quoted
            // skip opening quote
            pos++;
            int endPos = ba.indexOf('"', pos);
            // skip quoted quotes
            while (endPos >= 0 && ba.at(endPos - 1) == '\\') {
                endPos = ba.indexOf('"', endPos + 1);
            }
            if (endPos < 0) {
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
            if (endPos < 0) {
                endPos = ba.size();
            }
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
    for (auto itr = map.begin(); itr != map.end(); itr++) {
        if (!ba.isEmpty()) {
            ba.append(',');
        }
        ba.append(itr.key() + QByteArrayLiteral("="));
        auto value = itr.value();
        const char *separators = "()<>@,;:\\\"/[]?={} \t";
        bool quote = false;
        for (const char *c = separators; *c; c++) {
            if (value.contains(*c)) {
                quote = true;
                break;
            }
        }
        if (quote) {
            value.replace('\\', QByteArrayLiteral("\\\\"));
            value.replace('\"', QByteArrayLiteral("\\\""));
            ba.append('"' + value + '"');
        } else {
            ba.append(value);
        }
    }
    return ba;
}
