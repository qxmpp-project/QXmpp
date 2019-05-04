/*
 * Copyright (C) 2008-2019 The QXmpp developers
 *
 * Authors:
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

#ifndef QXMPPHTTPUPLOADIQ_H
#define QXMPPHTTPUPLOADIQ_H

#include <QMap>
#include <QMimeType>
#include <QUrl>

#include "QXmppIq.h"

class QXmppHttpUploadRequestIqPrivate;
class QXmppHttpUploadSlotIqPrivate;

/// \brief Represents an HTTP File Upload IQ for requesting an upload slot as
/// defined by XEP-0363: HTTP File Upload [v0.9.0].
///
/// \ingroup Stanzas

class QXMPP_EXPORT QXmppHttpUploadRequestIq : public QXmppIq
{
public:
    QXmppHttpUploadRequestIq();
    ~QXmppHttpUploadRequestIq();

    QString fileName() const;
    void setFileName(const QString &filename);

    qint64 size() const;
    void setSize(qint64 size);

    QMimeType contentType() const;
    void setContentType(const QMimeType &type);

    static bool isHttpUploadRequestIq(const QDomElement &element);

protected:
    /// \cond
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;
    /// \endcond

private:
    QXmppHttpUploadRequestIqPrivate* const d;
};

/// \brief Represents an HTTP File Upload IQ result for receiving an upload slot as
/// defined by XEP-0363: HTTP File Upload [v0.9.0].
///
/// \ingroup Stanzas

class QXMPP_EXPORT QXmppHttpUploadSlotIq : public QXmppIq
{
public:
    QXmppHttpUploadSlotIq();
    ~QXmppHttpUploadSlotIq();

    QUrl putUrl() const;
    void setPutUrl(const QUrl &putUrl);

    QUrl getUrl() const;
    void setGetUrl(const QUrl &getUrl);

    QMap<QString, QString> putHeaders() const;
    void setPutHeaders(const QMap<QString, QString> &putHeaders);

    static bool isHttpUploadSlotIq(const QDomElement &element);

protected:
    /// \cond
    void parseElementFromChild(const QDomElement &element);
    void toXmlElementFromChild(QXmlStreamWriter *writer) const;
    /// \endcond

private:
    QXmppHttpUploadSlotIqPrivate* const d;
};

#endif // QXMPPHTTPUPLOADIQ_H
