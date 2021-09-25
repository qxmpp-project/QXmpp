/*
 * Copyright (C) 2008-2021 The QXmpp developers
 *
 * Author:
 *  Linus Jahn
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

#include "QXmppPubSubEvent.h"

#include "QXmppConstants_p.h"
#include "QXmppDataForm.h"
#include "QXmppUtils.h"

#include <QDomElement>

///
/// \class QXmppPubSubEventBase
///
/// The QXmppPubSubEventBase class is an abstract class used for parsing of
/// generic PubSub event notifications as defined by \xep{0060, Publish-
/// Subscribe}.
///
/// This class cannot be used directly. For a full-featured access to the event
/// notifications, please use the QXmppPubSubEvent class.
///
/// \since QXmpp 1.5
///

///
/// \class QXmppPubSubEvent
///
/// \brief The QXmppPubSubEvent class represents a PubSub event notification as
/// defined by \xep{0060, Publish-Subscribe}.
///
/// This class has a template parameter that can be used to define the type of
/// the contained items.
///
/// You can use QXmppPubSubEvent::isPubSubItem() to check whether an DOM element
/// is a &lt;message/&gt; with a PubSub event notification. If you set a special
/// type as a template parameter, validity of the items will also be checked. To
/// check for an event notification with items from \xep{0118, User Tune} for
/// example, you could use the following:
/// \code
/// QXmppPubSubEvent<QXmppTuneItem>::isPubSubEvent(element);
/// \endcode
///
/// \ingroup Stanzas
///
/// \since QXmpp 1.5
///

static const QStringList PUBSUB_EVENTS = {
    QStringLiteral("configuration"),
    QStringLiteral("delete"),
    QStringLiteral("items"),
    QStringLiteral("purge"),
    QStringLiteral("subscription"),
};

static QDomElement firstChildElement(const QDomElement &element, const QString &tagName, const QString &namespaceURI)
{
    for (QDomNode child = element.firstChild(); !child.isNull(); child = child.nextSibling()) {
        if (child.isElement() && child.namespaceURI() == namespaceURI) {
            QDomElement elt = child.toElement();
            if (tagName.isEmpty() || elt.tagName() == tagName) {
                return elt;
            }
        }
    }
    return QDomElement();
}

class QXmppPubSubEventPrivate : public QSharedData
{
public:
    QXmppPubSubEventPrivate(QXmppPubSubEventBase::EventType type,
                            const QString &node);

    QXmppPubSubEventBase::EventType eventType;
    QString node;
    QStringList retractIds;
    QString redirectUri;
    std::optional<QXmppPubSubSubscription> subscription;
    std::optional<QXmppDataForm> configurationForm;
};

QXmppPubSubEventPrivate::QXmppPubSubEventPrivate(QXmppPubSubEventBase::EventType type,
                                                 const QString &node)
    : eventType(type), node(node)
{
}

///
/// Constructs a PubSub event.
///
QXmppPubSubEventBase::QXmppPubSubEventBase(EventType type, const QString &node)
    : d(new QXmppPubSubEventPrivate(type, node))
{
    setType(QXmppMessage::Normal);
}

/// Default copy-constructor.
QXmppPubSubEventBase::QXmppPubSubEventBase(const QXmppPubSubEventBase &other) = default;

QXmppPubSubEventBase::~QXmppPubSubEventBase() = default;

/// Default assignment operator.
QXmppPubSubEventBase &QXmppPubSubEventBase::operator=(const QXmppPubSubEventBase &other) = default;

///
/// Returns the event type of the PubSub event.
///
QXmppPubSubEventBase::EventType QXmppPubSubEventBase::eventType() const
{
    return d->eventType;
}

///
/// Sets the event type of the PubSub event.
///
void QXmppPubSubEventBase::setEventType(EventType type)
{
    d->eventType = type;
}

///
/// Returns the name of the event's node.
///
/// This does not work with Subscription events. In those cases you need to get
/// the node of the subscription.
///
/// \sa subscription()
/// \sa QXmppPubSubSubscription::node()
///
QString QXmppPubSubEventBase::node() const
{
    return d->node;
}

///
/// Sets the name of the event's node.
///
/// This does not work with Subscription events. In those cases you need to set
/// the node of the subscription.
///
/// \sa subscription()
/// \sa QXmppPubSubSubscription::setNode()
///
void QXmppPubSubEventBase::setNode(const QString &node)
{
    d->node = node;
}

///
/// Returns the item IDs that have been retracted.
///
/// This is only used for the Items event type.
///
QStringList QXmppPubSubEventBase::retractIds() const
{
    return d->retractIds;
}

///
/// Sets the item IDs that have been retracted.
///
/// This is only used for the Items event type.
///
void QXmppPubSubEventBase::setRetractIds(const QStringList &retractIds)
{
    d->retractIds = retractIds;
}

///
/// Returns the redirect URI to the new node.
///
/// This can be set for delete notifications to inform subscribers of the new
/// node. Inclusion of this is of course optional.
///
QString QXmppPubSubEventBase::redirectUri() const
{
    return d->redirectUri;
}

///
/// Sets the redirect URI to the new node.
///
/// This can be set for delete notifications to inform subscribers of the new
/// node. Inclusion of this is of course optional.
///
void QXmppPubSubEventBase::setRedirectUri(const QString &redirectUri)
{
    d->redirectUri = redirectUri;
}

///
/// Returns the subscription in case of a Subscription event.
///
std::optional<QXmppPubSubSubscription> QXmppPubSubEventBase::subscription() const
{
    return d->subscription;
}

///
/// Sets the subscription in case of a Subscription event.
///
void QXmppPubSubEventBase::setSubscription(const std::optional<QXmppPubSubSubscription> &subscription)
{
    d->subscription = subscription;
}

///
/// Returns a configuration data form if the event contains one.
///
std::optional<QXmppDataForm> QXmppPubSubEventBase::configurationForm() const
{
    return d->configurationForm;
}

///
/// Sets a configuration data form (or clears it with std::nullopt).
///
void QXmppPubSubEventBase::setConfigurationForm(const std::optional<QXmppDataForm> &configurationForm)
{
    d->configurationForm = configurationForm;
}

/// \cond
bool QXmppPubSubEventBase::isPubSubEvent(const QDomElement &stanza, std::function<bool(const QDomElement &)> isItemValid)
{
    if (stanza.tagName() != QStringLiteral("message")) {
        return false;
    }

    // find correct "event" element
    auto event = firstChildElement(stanza, QStringLiteral("event"), ns_pubsub_event);
    auto eventTypeElement = event.firstChildElement();

    // check for validity of the event type
    EventType eventType;
    if (const auto index = PUBSUB_EVENTS.indexOf(eventTypeElement.tagName());
        index != -1) {
        eventType = EventType(index);
    } else {
        return false;
    }

    // check for "node" attribute when required
    switch (eventType) {
    case Delete:
    case Items:
    case Purge:
        if (!eventTypeElement.hasAttribute(QStringLiteral("node"))) {
            return false;
        }
        break;
    case Configuration:
    case Subscription:
        break;
    }

    // check individual content
    switch (eventType) {
    case Delete: {
        if (const auto redirect = eventTypeElement.firstChildElement(QStringLiteral("redirect"));
            !redirect.isNull() && !redirect.hasAttribute(QStringLiteral("uri"))) {
            return false;
        }
        break;
    }
    case Items: {
        // check validity of the items using isItemValid()
        for (auto itemElement = eventTypeElement.firstChildElement(QStringLiteral("item"));
             !itemElement.isNull();
             itemElement = itemElement.nextSiblingElement(QStringLiteral("item"))) {
            if (!isItemValid(itemElement)) {
                return false;
            }
        }
        break;
    }
    case Subscription: {
        if (!QXmppPubSubSubscription::isSubscription(eventTypeElement)) {
            return false;
        }
    }
    case Configuration:
    case Purge:
        break;
    }

    return true;
}

bool QXmppPubSubEventBase::parseExtension(const QDomElement &eventElement, QXmpp::SceMode sceMode)
{
    if (sceMode & QXmpp::SceSensitive &&
            eventElement.tagName() == QStringLiteral("event") &&
            eventElement.namespaceURI() == ns_pubsub_event) {
        // check that the query type is valid
        const auto eventTypeElement = eventElement.firstChildElement();
        if (const auto index = PUBSUB_EVENTS.indexOf(eventTypeElement.tagName());
            index != -1) {
            d->eventType = EventType(index);
        } else {
            return false;
        }

        // parse "node" attribute
        switch (d->eventType) {
        case Configuration:
        case Delete:
        case Items:
        case Purge:
            d->node = eventTypeElement.attribute(QStringLiteral("node"));
            break;
        case Subscription:
            break;
        }

        // check the items using isItemValid()
        switch (d->eventType) {
        case Delete:
            if (auto redirect = eventTypeElement.firstChildElement(QStringLiteral("redirect"));
                !redirect.isNull()) {
                d->redirectUri = redirect.attribute(QStringLiteral("uri"));
            }
            break;
        case Items:
            // parse items
            parseItems(eventTypeElement);

            // parse retract ids
            for (auto retract = eventTypeElement.firstChildElement(QStringLiteral("retract"));
                 !retract.isNull();
                 retract = retract.nextSiblingElement(QStringLiteral("retract"))) {
                d->retractIds << retract.attribute(QStringLiteral("id"));
            }
            break;
        case Subscription: {
            QXmppPubSubSubscription subscription;
            subscription.parse(eventTypeElement);
            d->subscription = subscription;
            break;
        }
        case Configuration:
            if (auto formElement = firstChildElement(eventTypeElement, QStringLiteral("x"), ns_data);
                !formElement.isNull()) {
                QXmppDataForm form;
                form.parse(formElement);
                d->configurationForm = form;
            }
            break;
        case Purge:
            break;
        }
    } else {
        // handles QXmppMessage default extensions
        return QXmppMessage::parseExtension(eventElement, sceMode);
    }

    return true;
}

void QXmppPubSubEventBase::serializeExtensions(QXmlStreamWriter *writer, QXmpp::SceMode sceMode, const QString &baseNamespace) const
{
    QXmppMessage::serializeExtensions(writer, sceMode, baseNamespace);

    if (!(sceMode & QXmpp::SceSensitive)) {
        return;
    }

    writer->writeStartElement(QStringLiteral("event"));
    writer->writeDefaultNamespace(ns_pubsub_event);

    if (d->eventType == Subscription && d->subscription) {
        d->subscription->toXml(writer);
    } else {
        writer->writeStartElement(PUBSUB_EVENTS.at(int(d->eventType)));

        // write node attribute
        switch (d->eventType) {
        case Delete:
        case Items:
        case Purge:
            // node attribute is required
            writer->writeAttribute(QStringLiteral("node"), d->node);
            break;
        case Configuration:
            // node attribute is optional
            helperToXmlAddAttribute(writer, QStringLiteral("node"), d->node);
            break;
        case Subscription:
            break;
        }

        switch (d->eventType) {
        case Configuration:
            if (d->configurationForm) {
                d->configurationForm->toXml(writer);
            }
            break;
        case Delete:
            if (!d->redirectUri.isEmpty()) {
                writer->writeStartElement(QStringLiteral("redirect"));
                writer->writeAttribute(QStringLiteral("uri"), d->redirectUri);
                writer->writeEndElement();
            }
        case Items:
            // serialize items
            serializeItems(writer);

            // serialize retract ids
            for (const auto &id : std::as_const(d->retractIds)) {
                writer->writeStartElement(QStringLiteral("retract"));
                writer->writeAttribute(QStringLiteral("id"), id);
                writer->writeEndElement();
            }

            break;
        case Purge:
        case Subscription:
            break;
        }

        writer->writeEndElement();  // close event's type element
    }
    writer->writeEndElement();  // </event>
}
/// \endcond
