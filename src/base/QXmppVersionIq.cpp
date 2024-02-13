// SPDX-FileCopyrightText: 2010 Jeremy Lainé <jeremy.laine@m4x.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppVersionIq.h"

#include "QXmppConstants_p.h"
#include "QXmppUtils_p.h"

#include <QDomElement>

using namespace QXmpp::Private;

/// Returns the name of the software.
///

QString QXmppVersionIq::name() const
{
    return m_name;
}

/// Sets the name of the software.
///
/// \param name

void QXmppVersionIq::setName(const QString &name)
{
    m_name = name;
}

/// Returns the operating system.
///

QString QXmppVersionIq::os() const
{
    return m_os;
}

/// Sets the operating system.
///
/// \param os

void QXmppVersionIq::setOs(const QString &os)
{
    m_os = os;
}

/// Returns the software version.
///

QString QXmppVersionIq::version() const
{
    return m_version;
}

/// Sets the software version.
///
/// \param version

void QXmppVersionIq::setVersion(const QString &version)
{
    m_version = version;
}

/// \cond
bool QXmppVersionIq::isVersionIq(const QDomElement &element)
{
    return isIqType(element, u"query", ns_version);
}

bool QXmppVersionIq::checkIqType(const QString &tagName, const QString &xmlNamespace)
{
    return tagName == "query" && xmlNamespace == ns_version;
}

void QXmppVersionIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement queryElement = element.firstChildElement(QStringLiteral("query"));
    m_name = queryElement.firstChildElement(QStringLiteral("name")).text();
    m_os = queryElement.firstChildElement(QStringLiteral("os")).text();
    m_version = queryElement.firstChildElement(QStringLiteral("version")).text();
}

void QXmppVersionIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement(QStringLiteral("query"));
    writer->writeDefaultNamespace(ns_version);

    if (!m_name.isEmpty()) {
        writeXmlTextElement(writer, u"name", m_name);
    }

    if (!m_os.isEmpty()) {
        writeXmlTextElement(writer, u"os", m_os);
    }

    if (!m_version.isEmpty()) {
        writeXmlTextElement(writer, u"version", m_version);
    }

    writer->writeEndElement();
}
/// \endcond
