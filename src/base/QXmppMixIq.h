/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Author:
 *  Linus Jahn <lnj@kaidan.im>
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

#ifndef QXMPPMIXIQ_H
#define QXMPPMIXIQ_H

#include "QXmppIq.h"

class QXmppMixIqPrivate;

/// \brief The QXmppMixIq class represents an IQ used to do actions on a MIX
/// channel as defined by XEP-0369: Mediated Information eXchange (MIX)
/// (v0.14.1) and XEP-0405: Mediated Information eXchange (MIX): Participant
/// Server Requirements (v0.3.1).
///
/// \ingroup Stanzas

class QXMPP_EXPORT QXmppMixIq : public QXmppIq
{
public:
    enum Type {
        None,
        ClientJoin,
        ClientLeave,
        Join,
        Leave,
        UpdateSubscription,
        SetNick,
        Create,
        Destroy
    };

    QXmppMixIq();
    ~QXmppMixIq();

    QXmppMixIq::Type actionType() const;
    void setActionType(QXmppMixIq::Type);

    QString jid() const;
    void setJid(const QString&);

    QString channelName() const;
    void setChannelName(const QString&);

    QStringList nodes() const;
    void setNodes(const QStringList&);

    QString nick() const;
    void setNick(const QString&);

    /// \cond
    static bool isMixIq(const QDomElement&);
    /// \endcond

protected:
    /// \cond
    void parseElementFromChild(const QDomElement&);
    void toXmlElementFromChild(QXmlStreamWriter*) const;
    /// \endcond

private:
    QXmppMixIqPrivate *d;
};

#endif // QXMPPMIXIQ_H
