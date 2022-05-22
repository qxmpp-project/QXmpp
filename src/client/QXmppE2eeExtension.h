// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPE2EEEXTENSION_H
#define QXMPPE2EEEXTENSION_H

#include "QXmppExtension.h"
#include "QXmppSendResult.h"
#include "QXmppSendStanzaParams.h"

#include <optional>

class QDomElement;
class QXmppMessage;
class QXmppIq;
template<typename T>
class QFuture;

class QXmppE2eeExtension : public QXmppExtension
{
public:
    struct NotEncrypted
    {
    };

    using MessageEncryptResult = std::variant<QByteArray, QXmpp::SendError>;
    using IqEncryptResult = std::variant<QByteArray, QXmpp::SendError>;
    using IqDecryptResult = std::variant<QDomElement, NotEncrypted, QXmpp::SendError>;

    virtual QFuture<MessageEncryptResult> encryptMessage(QXmppMessage &&, const std::optional<QXmppSendStanzaParams> &) = 0;

    virtual QFuture<IqEncryptResult> encryptIq(QXmppIq &&, const std::optional<QXmppSendStanzaParams> &) = 0;
    virtual QFuture<IqDecryptResult> decryptIq(const QDomElement &) = 0;
};

#endif  // QXMPPE2EEEXTENSION_H
