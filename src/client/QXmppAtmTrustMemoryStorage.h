// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPATMTRUSTMEMORYSTORAGE_H
#define QXMPPATMTRUSTMEMORYSTORAGE_H

#include "QXmppAtmTrustStorage.h"
#include "QXmppTrustMemoryStorage.h"

class QXmppAtmTrustMemoryStoragePrivate;

class QXMPP_EXPORT QXmppAtmTrustMemoryStorage : virtual public QXmppAtmTrustStorage, public QXmppTrustMemoryStorage
{
public:
    QXmppAtmTrustMemoryStorage();
    ~QXmppAtmTrustMemoryStorage();

    /// \cond
    QFuture<void> addKeysForPostponedTrustDecisions(const QString &encryption, const QByteArray &senderKeyId, const QList<QXmppTrustMessageKeyOwner> &keyOwners) override;
    QFuture<void> removeKeysForPostponedTrustDecisions(const QString &encryption, const QList<QByteArray> &keyIdsForAuthentication, const QList<QByteArray> &keyIdsForDistrusting) override;
    QFuture<void> removeKeysForPostponedTrustDecisions(const QString &encryption, const QList<QByteArray> &senderKeyIds) override;
    QFuture<void> removeKeysForPostponedTrustDecisions(const QString &encryption) override;
    QFuture<QHash<bool, QMultiHash<QString, QByteArray>>> keysForPostponedTrustDecisions(const QString &encryption, const QList<QByteArray> &senderKeyIds = {}) override;

    QFuture<void> resetAll(const QString &encryption) override;
    /// \endcond

private:
    std::unique_ptr<QXmppAtmTrustMemoryStoragePrivate> d;
};

#endif  // QXMPPATMTRUSTMEMORYSTORAGE_H
