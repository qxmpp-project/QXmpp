// SPDX-FileCopyrightText: 2009 Manjeet Dahiya <manjeetdahiya@gmail.com>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPPRESENCE_H
#define QXMPPPRESENCE_H

#include "QXmppJingleIq.h"
#include "QXmppMucIq.h"
#include "QXmppStanza.h"

class QXmppPresencePrivate;

///
/// \brief The QXmppPresence class represents an XMPP presence stanza.
///
/// \ingroup Stanzas
///
class QXMPP_EXPORT QXmppPresence : public QXmppStanza
{
public:
    /// This enum is used to describe a presence type.
    enum Type {
        Error = 0,     ///< An error has occurred regarding processing or delivery of a previously-sent presence stanza.
        Available,     ///< Signals that the sender is online and available for communication.
        Unavailable,   ///< Signals that the sender is no longer available for communication.
        Subscribe,     ///< The sender wishes to subscribe to the recipient's  presence.
        Subscribed,    ///< The sender has allowed the recipient to receive their presence.
        Unsubscribe,   ///< The sender is unsubscribing from another entity's presence.
        Unsubscribed,  ///< The subscription request has been denied or a previously-granted subscription has been cancelled.
        Probe          ///< A request for an entity's current presence; SHOULD be generated only by a server on behalf of a user.
    };

    /// This enum is used to describe an availability status.
    enum AvailableStatusType {
        Online = 0,  ///< The entity or resource is online.
        Away,        ///< The entity or resource is temporarily away.
        XA,          ///< The entity or resource is away for an extended period.
        DND,         ///< The entity or resource is busy ("Do Not Disturb").
        Chat,        ///< The entity or resource is actively interested in chatting.
        Invisible    ///< obsolete \xep{0018}: Invisible Presence
    };

    /// This enum is used to describe vCard updates as defined by
    /// \xep{0153}: vCard-Based Avatars
    enum VCardUpdateType {
        VCardUpdateNone = 0,    ///< Protocol is not supported
        VCardUpdateNoPhoto,     ///< User is not using any image
        VCardUpdateValidPhoto,  ///< User is advertising an image
        VCardUpdateNotReady     ///< User is not ready to advertise an image

        /// \note This enables recipients to distinguish between the absence of an image
        /// (empty photo element) and mere support for the protocol (empty update child).
    };

    QXmppPresence(QXmppPresence::Type type = QXmppPresence::Available);
    QXmppPresence(const QXmppPresence &other);
    QXmppPresence(QXmppPresence &&);
    ~QXmppPresence() override;

    QXmppPresence &operator=(const QXmppPresence &other);
    QXmppPresence &operator=(QXmppPresence &&);

    bool isXmppStanza() const override;

    AvailableStatusType availableStatusType() const;
    void setAvailableStatusType(AvailableStatusType type);

    int priority() const;
    void setPriority(int priority);

    QXmppPresence::Type type() const;
    void setType(QXmppPresence::Type);

    QString statusText() const;
    void setStatusText(const QString &statusText);

    // XEP-0045: Multi-User Chat
    QXmppMucItem mucItem() const;
    void setMucItem(const QXmppMucItem &item);

    QString mucPassword() const;
    void setMucPassword(const QString &password);

    QList<int> mucStatusCodes() const;
    void setMucStatusCodes(const QList<int> &codes);

    bool isMucSupported() const;
    void setMucSupported(bool supported);

    // XEP-0153: vCard-Based Avatars
    QByteArray photoHash() const;
    void setPhotoHash(const QByteArray &);

    VCardUpdateType vCardUpdateType() const;
    void setVCardUpdateType(VCardUpdateType type);

    // XEP-0115: Entity Capabilities
    QString capabilityHash() const;
    void setCapabilityHash(const QString &);

    QString capabilityNode() const;
    void setCapabilityNode(const QString &);

    QByteArray capabilityVer() const;
    void setCapabilityVer(const QByteArray &);

    QStringList capabilityExt() const;

    // XEP-0272: Multiparty Jingle (Muji)
    bool isPreparingMujiSession() const;
    void setIsPreparingMujiSession(bool isPreparingMujiSession);

    QVector<QXmppJingleIq::Content> mujiContents() const;
    void setMujiContents(const QVector<QXmppJingleIq::Content> &mujiContents);

    // XEP-0319: Last User Interaction in Presence
    QDateTime lastUserInteraction() const;
    void setLastUserInteraction(const QDateTime &);

    // XEP-0405: Mediated Information eXchange (MIX): Participant Server Requirements
    QString mixUserJid() const;
    void setMixUserJid(const QString &);

    QString mixUserNick() const;
    void setMixUserNick(const QString &);

    /// \cond
    void parse(const QDomElement &element) override;
    void toXml(QXmlStreamWriter *writer) const override;
    /// \endcond

private:
    /// \cond
    void parseExtension(const QDomElement &element, QXmppElementList &unknownElements);
    /// \endcond

    QSharedDataPointer<QXmppPresencePrivate> d;
};

#endif  // QXMPPPRESENCE_H
