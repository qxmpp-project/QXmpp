// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppMixIq.h"

#include "QXmppConstants_p.h"
#include "QXmppUtils_p.h"

#include <QDomElement>
#include <QSharedData>

using namespace QXmpp::Private;

static const QStringList MIX_ACTION_TYPES = {
    QString(),
    QStringLiteral("client-join"),
    QStringLiteral("client-leave"),
    QStringLiteral("join"),
    QStringLiteral("leave"),
    QStringLiteral("update-subscription"),
    QStringLiteral("setnick"),
    QStringLiteral("create"),
    QStringLiteral("destroy")
};

class QXmppMixIqPrivate : public QSharedData
{
public:
    QString jid;
    QString channelName;
    QStringList nodes;
    QString nick;
    QXmppMixIq::Type actionType = QXmppMixIq::None;
};

///
/// \class QXmppMixIq
///
/// This class represents an IQ used to do actions on a MIX channel as defined by
/// \xep{0369, Mediated Information eXchange (MIX)} and
/// \xep{0405, Mediated Information eXchange (MIX): Participant Server Requirements}.
///
/// \since QXmpp 1.1
///
/// \ingroup Stanzas
///

///
/// \enum QXmppMixIq::Type
///
/// Action type of the MIX IQ stanza.
///
/// \var QXmppMixIq::None
///
/// Nothing is done.
///
/// \var QXmppMixIq::ClientJoin
///
/// The client sends a request to join a MIX channel to the user's server.
///
/// \var QXmppMixIq::ClientLeave
///
/// The client sends a request to leave a MIX channel to the user's server.
///
/// \var QXmppMixIq::Join
///
/// The user's server forwards a join request from the client to the MIX channel.
///
/// \var QXmppMixIq::Leave
///
/// The user's server forwards a leave request from the client to the MIX channel.
///
/// \var QXmppMixIq::UpdateSubscription
///
/// The client subscribes to MIX nodes or unsubscribes from MIX nodes.
///
/// \var QXmppMixIq::SetNick
///
/// The client changes the user's nickname within the MIX channel.
///
/// \var QXmppMixIq::Create
///
/// The client creates a MIX channel.
///
/// \var QXmppMixIq::Destroy
///
/// The client destroys a MIX channel.
///

QXmppMixIq::QXmppMixIq()
    : d(new QXmppMixIqPrivate)
{
}

/// Default copy-constructor
QXmppMixIq::QXmppMixIq(const QXmppMixIq &) = default;
/// Default move-constructor
QXmppMixIq::QXmppMixIq(QXmppMixIq &&) = default;
QXmppMixIq::~QXmppMixIq() = default;
/// Default assignment operator
QXmppMixIq &QXmppMixIq::operator=(const QXmppMixIq &) = default;
/// Default move-assignment operator
QXmppMixIq &QXmppMixIq::operator=(QXmppMixIq &&) = default;

/// Returns the channel JID. It also contains a participant id for Join/
/// ClientJoin results.

QString QXmppMixIq::jid() const
{
    return d->jid;
}

/// Sets the channel JID. For results of Join/ClientJoin queries this also
/// needs to contain a participant id.

void QXmppMixIq::setJid(const QString &jid)
{
    d->jid = jid;
}

/// Returns the channel name (the name part of the channel JID). This may still
/// be empty, if a JID was set.

QString QXmppMixIq::channelName() const
{
    return d->channelName;
}

/// Sets the channel name for creating/destroying specific channels. When you
/// create a new channel, this can also be left empty to let the server
/// generate a name.

void QXmppMixIq::setChannelName(const QString &channelName)
{
    d->channelName = channelName;
}

/// Returns the list of nodes to subscribe to.

QStringList QXmppMixIq::nodes() const
{
    return d->nodes;
}

/// Sets the nodes to subscribe to. Note that for UpdateSubscription queries
/// you only need to include the new subscriptions.

void QXmppMixIq::setNodes(const QStringList &nodes)
{
    d->nodes = nodes;
}

/// Returns the user's nickname in the channel.

QString QXmppMixIq::nick() const
{
    return d->nick;
}

/// Sets the nickname for the channel.

void QXmppMixIq::setNick(const QString &nick)
{
    d->nick = nick;
}

/// Returns the MIX channel action type.

QXmppMixIq::Type QXmppMixIq::actionType() const
{
    return d->actionType;
}

/// Sets the channel action.

void QXmppMixIq::setActionType(QXmppMixIq::Type type)
{
    d->actionType = type;
}

/// \cond
bool QXmppMixIq::isMixIq(const QDomElement &element)
{
    const QDomElement &child = element.firstChildElement();
    return !child.isNull() && (child.namespaceURI() == ns_mix || child.namespaceURI() == ns_mix_pam);
}

void QXmppMixIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement child = element.firstChildElement();
    // determine action type
    if (auto index = MIX_ACTION_TYPES.indexOf(child.tagName()); index >= 0) {
        d->actionType = Type(index);
    }

    if (child.namespaceURI() == ns_mix_pam) {
        if (child.hasAttribute(QStringLiteral("channel"))) {
            d->jid = child.attribute(QStringLiteral("channel"));
        }

        child = child.firstChildElement();
    }

    if (!child.isNull() && child.namespaceURI() == ns_mix) {
        if (child.hasAttribute(QStringLiteral("jid"))) {
            d->jid = child.attribute(QStringLiteral("jid"));
        }
        if (child.hasAttribute(QStringLiteral("channel"))) {
            d->channelName = child.attribute(QStringLiteral("channel"));
        }

        for (const auto &node : iterChildElements(child, u"subscribe")) {
            d->nodes << node.attribute(QStringLiteral("node"));
        }
        d->nick = firstChildElement(child, u"nick").text();
    }
}

void QXmppMixIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    if (d->actionType == None) {
        return;
    }

    writer->writeStartElement(MIX_ACTION_TYPES.at(d->actionType));
    if (d->actionType == ClientJoin || d->actionType == ClientLeave) {
        writer->writeDefaultNamespace(toString65(ns_mix_pam));
        if (type() == Set) {
            writeOptionalXmlAttribute(writer, u"channel", d->jid);
        }

        if (d->actionType == ClientJoin) {
            writer->writeStartElement(QSL65("join"));
        } else if (d->actionType == ClientLeave) {
            writer->writeStartElement(QSL65("leave"));
        }
    }

    writer->writeDefaultNamespace(toString65(ns_mix));
    writeOptionalXmlAttribute(writer, u"channel", d->channelName);
    if (type() == Result) {
        writeOptionalXmlAttribute(writer, u"jid", d->jid);
    }

    for (const auto &node : d->nodes) {
        writer->writeStartElement(QSL65("subscribe"));
        writer->writeAttribute(QSL65("node"), node);
        writer->writeEndElement();
    }
    if (!d->nick.isEmpty()) {
        writer->writeTextElement(QSL65("nick"), d->nick);
    }

    writer->writeEndElement();
    if (d->actionType == ClientJoin || d->actionType == ClientLeave) {
        writer->writeEndElement();
    }
}
/// \endcond
