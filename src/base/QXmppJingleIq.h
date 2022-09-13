// SPDX-FileCopyrightText: 2010 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPJINGLEIQ_H
#define QXMPPJINGLEIQ_H

#include "QXmppIq.h"

#include <QHostAddress>

class QXmppJingleCandidatePrivate;
class QXmppJingleIqContentPrivate;
class QXmppJingleIqPrivate;
class QXmppJinglePayloadTypePrivate;
class QXmppJingleRtpFeedbackPropertyPrivate;
class QXmppSdpParameterPrivate;

class QXMPP_EXPORT QXmppSdpParameter
{
public:
    QXmppSdpParameter();

    QXMPP_PRIVATE_DECLARE_RULE_OF_SIX(QXmppSdpParameter)

    QString name() const;
    void setName(const QString &name);

    QString value() const;
    void setValue(const QString &value);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
    /// \endcond

    static bool isSdpParameter(const QDomElement &element);

private:
    QSharedDataPointer<QXmppSdpParameterPrivate> d;
};

class QXMPP_EXPORT QXmppJingleRtpFeedbackProperty
{
public:
    QXmppJingleRtpFeedbackProperty();

    QXMPP_PRIVATE_DECLARE_RULE_OF_SIX(QXmppJingleRtpFeedbackProperty)

    QString type() const;
    void setType(const QString &type);

    QString subtype() const;
    void setSubtype(const QString &subtype);

    QVector<QXmppSdpParameter> parameters() const;
    void setParameters(const QVector<QXmppSdpParameter> &parameters);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
    /// \endcond

    static bool isJingleRtpFeedbackProperty(const QDomElement &element);

private:
    QSharedDataPointer<QXmppJingleRtpFeedbackPropertyPrivate> d;
};

class QXMPP_EXPORT QXmppJingleRtpFeedbackInterval
{
public:
    QXmppJingleRtpFeedbackInterval();

    QXMPP_PRIVATE_DECLARE_RULE_OF_SIX(QXmppJingleRtpFeedbackInterval)

    uint64_t value() const;
    void setValue(uint64_t value);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
    /// \endcond

    static bool isJingleRtpFeedbackInterval(const QDomElement &element);

private:
    uint64_t m_value;
};

///
/// \brief The QXmppJinglePayloadType class represents a payload type
/// as specified by \xep{0167}: Jingle RTP Sessions and RFC 5245.
///
class QXMPP_EXPORT QXmppJinglePayloadType
{
public:
    QXmppJinglePayloadType();
    QXmppJinglePayloadType(const QXmppJinglePayloadType &other);
    ~QXmppJinglePayloadType();

    unsigned char channels() const;
    void setChannels(unsigned char channels);

    unsigned int clockrate() const;
    void setClockrate(unsigned int clockrate);

    unsigned char id() const;
    void setId(unsigned char id);

    unsigned int maxptime() const;
    void setMaxptime(unsigned int maxptime);

    QString name() const;
    void setName(const QString &name);

    QMap<QString, QString> parameters() const;
    void setParameters(const QMap<QString, QString> &parameters);

    unsigned int ptime() const;
    void setPtime(unsigned int ptime);

    QVector<QXmppJingleRtpFeedbackProperty> rtpFeedbackProperties() const;
    void setRtpFeedbackProperties(const QVector<QXmppJingleRtpFeedbackProperty> &rtpFeedbackProperties);

    QVector<QXmppJingleRtpFeedbackInterval> rtpFeedbackIntervals() const;
    void setRtpFeedbackIntervals(const QVector<QXmppJingleRtpFeedbackInterval> &rtpFeedbackIntervals);

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;
    /// \endcond

    QXmppJinglePayloadType &operator=(const QXmppJinglePayloadType &other);
    bool operator==(const QXmppJinglePayloadType &other) const;

private:
    QSharedDataPointer<QXmppJinglePayloadTypePrivate> d;
};

///
/// \brief The QXmppJingleCandidate class represents a transport candidate
/// as specified by \xep{0176}: Jingle ICE-UDP Transport Method.
///
class QXMPP_EXPORT QXmppJingleCandidate
{
public:
    /// This enum is used to describe a candidate's type.
    enum Type {
        HostType,             ///< Host candidate, a local address/port.
        PeerReflexiveType,    ///< Peer-reflexive candidate,
                              ///< the address/port as seen from the peer.
        ServerReflexiveType,  ///< Server-reflexive candidate,
                              ///< the address/port as seen by the STUN server
        RelayedType           ///< Relayed candidate, a candidate from
                              ///< a TURN relay.
    };

    QXmppJingleCandidate();
    QXmppJingleCandidate(const QXmppJingleCandidate &other);
    QXmppJingleCandidate(QXmppJingleCandidate &&);
    ~QXmppJingleCandidate();

    QXmppJingleCandidate &operator=(const QXmppJingleCandidate &other);
    QXmppJingleCandidate &operator=(QXmppJingleCandidate &&);

    int component() const;
    void setComponent(int component);

    QString foundation() const;
    void setFoundation(const QString &foundation);

    int generation() const;
    void setGeneration(int generation);

    QHostAddress host() const;
    void setHost(const QHostAddress &host);

    QString id() const;
    void setId(const QString &id);

    int network() const;
    void setNetwork(int network);

    quint16 port() const;
    void setPort(quint16 port);

    int priority() const;
    void setPriority(int priority);

    QString protocol() const;
    void setProtocol(const QString &protocol);

    QXmppJingleCandidate::Type type() const;
    void setType(QXmppJingleCandidate::Type);

    bool isNull() const;

    /// \cond
    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

    static QXmppJingleCandidate::Type typeFromString(const QString &typeStr, bool *ok = nullptr);
    static QString typeToString(QXmppJingleCandidate::Type type);
    /// \endcond

private:
    QSharedDataPointer<QXmppJingleCandidatePrivate> d;
};

///
/// \brief The QXmppJingleIq class represents an IQ used for initiating media
/// sessions as specified by \xep{0166}: Jingle.
///
/// \ingroup Stanzas
///
class QXMPP_EXPORT QXmppJingleIq : public QXmppIq
{
public:
    /// This enum is used to describe a Jingle action.
    enum Action {
        ContentAccept,
        ContentAdd,
        ContentModify,
        ContentReject,
        ContentRemove,
        DescriptionInfo,
        SecurityInfo,
        SessionAccept,
        SessionInfo,
        SessionInitiate,
        SessionTerminate,
        TransportAccept,
        TransportInfo,
        TransportReject,
        TransportReplace
    };

    ///
    /// Contains the state of an RTP session as specified by \xep{0167, Jingle RTP Sessions}
    /// Informational Messages.
    ///
    /// \since QXmpp 1.5
    ///
    enum RtpSessionState {
        /// No session state specified
        None,
        /// Actively participating in the session after having been on mute or having put the other
        /// party on hold
        Active,
        /// Temporarily not listening for media from the other party
        Hold,
        /// Ending hold state
        Unhold,
        /// Temporarily not sending media to the other party but continuing to accept media from it
        Mute,
        /// Ending mute state
        Unmute,
        /// State after the callee acknowledged the call but did not yet interacted with it
        Ringing
    };

    /// \internal
    ///
    /// The QXmppJingleIq::Content class represents the "content" element of a
    /// QXmppJingleIq.
    ///
    class QXMPP_EXPORT Content
    {
    public:
        Content();
        Content(const QXmppJingleIq::Content &other);
        Content(QXmppJingleIq::Content &&);
        ~Content();

        Content &operator=(const Content &other);
        Content &operator=(Content &&);

        QString creator() const;
        void setCreator(const QString &creator);

        QString name() const;
        void setName(const QString &name);

        QString senders() const;
        void setSenders(const QString &senders);

        // XEP-0167: Jingle RTP Sessions
        QString descriptionMedia() const;
        void setDescriptionMedia(const QString &media);

        quint32 descriptionSsrc() const;
        void setDescriptionSsrc(quint32 ssrc);

        bool isRtpMultiplexingSupported() const;
        void setRtpMultiplexingSupported(bool isRtpMultiplexingSupported);

        void addPayloadType(const QXmppJinglePayloadType &payload);
        QList<QXmppJinglePayloadType> payloadTypes() const;
        void setPayloadTypes(const QList<QXmppJinglePayloadType> &payloadTypes);

        void addTransportCandidate(const QXmppJingleCandidate &candidate);
        QList<QXmppJingleCandidate> transportCandidates() const;
        void setTransportCandidates(const QList<QXmppJingleCandidate> &candidates);

        QString transportUser() const;
        void setTransportUser(const QString &user);

        QString transportPassword() const;
        void setTransportPassword(const QString &password);

        QVector<QXmppJingleRtpFeedbackProperty> rtpFeedbackProperties() const;
        void setRtpFeedbackProperties(const QVector<QXmppJingleRtpFeedbackProperty> &rtpFeedbackProperties);

        QVector<QXmppJingleRtpFeedbackInterval> rtpFeedbackIntervals() const;
        void setRtpFeedbackIntervals(const QVector<QXmppJingleRtpFeedbackInterval> &rtpFeedbackIntervals);

        // XEP-0320: Use of DTLS-SRTP in Jingle Sessions
        QByteArray transportFingerprint() const;
        void setTransportFingerprint(const QByteArray &fingerprint);

        QString transportFingerprintHash() const;
        void setTransportFingerprintHash(const QString &hash);

        QString transportFingerprintSetup() const;
        void setTransportFingerprintSetup(const QString &setup);

        /// \cond
        void parse(const QDomElement &element);
        void toXml(QXmlStreamWriter *writer) const;

        bool parseSdp(const QString &sdp);
        QString toSdp() const;
        /// \endcond

    private:
        QSharedDataPointer<QXmppJingleIqContentPrivate> d;
    };

    /// \internal
    ///
    /// The QXmppJingleIq::Reason class represents the "reason" element of a
    /// QXmppJingleIq.
    ///
    class QXMPP_EXPORT Reason
    {
    public:
        /// This enum is used to describe a reason's type.
        enum Type {
            None,
            AlternativeSession,
            Busy,
            Cancel,
            ConnectivityError,
            Decline,
            Expired,
            FailedApplication,
            FailedTransport,
            GeneralError,
            Gone,
            IncompatibleParameters,
            MediaError,
            SecurityError,
            Success,
            Timeout,
            UnsupportedApplications,
            UnsupportedTransports
        };

        Reason();

        QString text() const;
        void setText(const QString &text);

        Type type() const;
        void setType(Type type);

        /// \cond
        void parse(const QDomElement &element);
        void toXml(QXmlStreamWriter *writer) const;
        /// \endcond

    private:
        QString m_text;
        Type m_type;
    };

    QXmppJingleIq();
    QXmppJingleIq(const QXmppJingleIq &other);
    QXmppJingleIq(QXmppJingleIq &&);
    ~QXmppJingleIq() override;

    QXmppJingleIq &operator=(const QXmppJingleIq &other);
    QXmppJingleIq &operator=(QXmppJingleIq &&);

    Action action() const;
    void setAction(Action action);

    void addContent(const Content &content);
    QList<Content> contents() const;
    void setContents(const QList<Content> &contents);

    QString initiator() const;
    void setInitiator(const QString &initiator);

    Reason &reason();
    const Reason &reason() const;

    QString responder() const;
    void setResponder(const QString &responder);

    QString sid() const;
    void setSid(const QString &sid);

    QString mujiGroupChatJid() const;
    void setMujiGroupChatJid(const QString &mujiGroupChatJid);

    RtpSessionState rtpSessionState() const;
    void setRtpSessionState(RtpSessionState rtpSessionState);

#if QXMPP_DEPRECATED_SINCE(1, 5)
    bool ringing() const;
    void setRinging(bool ringing);
#endif

    /// \cond
    static bool isJingleIq(const QDomElement &element);
    /// \endcond

protected:
    /// \cond
    void parseElementFromChild(const QDomElement &element) override;
    void toXmlElementFromChild(QXmlStreamWriter *writer) const override;
    /// \endcond

private:
    QSharedDataPointer<QXmppJingleIqPrivate> d;
};

#endif
