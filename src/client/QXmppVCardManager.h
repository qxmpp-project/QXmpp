// SPDX-FileCopyrightText: 2009 Manjeet Dahiya <manjeetdahiya@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPVCARDMANAGER_H
#define QXMPPVCARDMANAGER_H

#include "QXmppClientExtension.h"

#include <variant>

template<typename T>
class QXmppTask;
class QXmppVCardIq;
class QXmppError;
class QXmppVCardManagerPrivate;

///
/// \brief The QXmppVCardManager class gets/sets XMPP vCards. It is an
/// implementation of \xep{0054}: vcard-temp.
///
/// \note Its object should not be created using its constructor. Instead
/// \c QXmppClient::findExtension<QXmppVCardManager>() should be used to get
/// the instantiated object of this class.
///
/// <B>Getting vCards of entries in Roster:</B><BR>
/// It doesn't store vCards of the JIDs in the roster of connected user. Instead
/// client has to request for a particular vCard using requestVCard(). And connect to
/// the signal vCardReceived() to get the requested vCard.
///
/// <B>Getting vCard of the connected client:</B><BR>
/// For getting the vCard of the connected user itself. Client can call requestClientVCard()
/// and on the signal clientVCardReceived() it can get its vCard using clientVCard().
///
/// <B>Setting vCard of the client:</B><BR>
/// Using setClientVCard() client can set its vCard.
///
/// \note Client can't set/change vCards of roster entries.
///
/// \ingroup Managers
///
class QXMPP_EXPORT QXmppVCardManager : public QXmppClientExtension
{
    Q_OBJECT

public:
    /// Iq result containing QXmppVCardIq or a QXmppError
    using IqResult = std::variant<QXmppVCardIq, QXmppError>;
    /// Simple result containing QXmpp::Success or a QXmppError
    using Result = std::variant<QXmpp::Success, QXmppError>;

    QXmppVCardManager();
    ~QXmppVCardManager() override;

    QXmppTask<IqResult> requestVCard(const QString &bareJid = QString());

    const QXmppVCardIq &clientVCard() const;
    QXmppTask<Result> setClientVCard(const QXmppVCardIq &);

    QXmppTask<IqResult> requestClientVCard();
    bool isClientVCardReceived() const;

    /// \cond
    QStringList discoveryFeatures() const override;
    /// \endcond

Q_SIGNALS:
    /// This signal is emitted when the requested vCard is received
    /// after calling the requestVCard() function.
    void vCardReceived(const QXmppVCardIq &);

    /// This signal is emitted when the client's vCard is received
    /// after calling the requestClientVCard() function.
    void clientVCardReceived();

private:
    void handleReceivedVCard(const QXmppVCardIq &vcard);

    const std::unique_ptr<QXmppVCardManagerPrivate> d;
};

#endif  // QXMPPVCARDMANAGER_H
