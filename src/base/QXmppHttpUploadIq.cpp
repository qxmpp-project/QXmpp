// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppHttpUploadIq.h"

#include "QXmppConstants_p.h"

#include <QDomElement>
#include <QMimeDatabase>
#include <QSharedData>
#include <QUrl>

class QXmppHttpUploadRequestIqPrivate : public QSharedData
{
public:
    QString fileName;
    qint64 size;
    QMimeType contentType;
};

QXmppHttpUploadRequestIq::QXmppHttpUploadRequestIq()
    : d(new QXmppHttpUploadRequestIqPrivate())
{
}

/// Default copy-constructor
QXmppHttpUploadRequestIq::QXmppHttpUploadRequestIq(const QXmppHttpUploadRequestIq &) = default;
/// Default move-constructor
QXmppHttpUploadRequestIq::QXmppHttpUploadRequestIq(QXmppHttpUploadRequestIq &&) = default;
QXmppHttpUploadRequestIq::~QXmppHttpUploadRequestIq() = default;
/// Default assignment operator
QXmppHttpUploadRequestIq &QXmppHttpUploadRequestIq::operator=(const QXmppHttpUploadRequestIq &) = default;
/// Default assignment operator
QXmppHttpUploadRequestIq &QXmppHttpUploadRequestIq::operator=(QXmppHttpUploadRequestIq &&) = default;

///
/// Returns the file name of the file to be uploaded.
///
QString QXmppHttpUploadRequestIq::fileName() const
{
    return d->fileName;
}

///
/// Sets the file name. The upload service will use this to create the upload/
/// download URLs. This may also differ from the actual file name to get a
/// different URL. It's not required to replace special characters (this is the
/// server's job).
///
void QXmppHttpUploadRequestIq::setFileName(const QString &fileName)
{
    d->fileName = fileName;
}

///
/// Returns the file's size in bytes.
///
qint64 QXmppHttpUploadRequestIq::size() const
{
    return d->size;
}

///
/// Sets the file's size in bytes.
///
void QXmppHttpUploadRequestIq::setSize(qint64 size)
{
    d->size = size;
}

///
/// Returns the (optional) MIME-type of the file.
///
QMimeType QXmppHttpUploadRequestIq::contentType() const
{
    return d->contentType;
}

///
/// Sets the MIME-type of the file. This is optional.
///
void QXmppHttpUploadRequestIq::setContentType(const QMimeType &type)
{
    d->contentType = type;
}

///
/// Returns true, if the the element is an HTTP File Upload slot request IQ.
///
bool QXmppHttpUploadRequestIq::isHttpUploadRequestIq(const QDomElement &element)
{
    if (element.tagName() == "iq") {
        QDomElement request = element.firstChildElement("request");
        return !request.isNull() && request.namespaceURI() == ns_http_upload;
    }
    return false;
}

/// \cond
void QXmppHttpUploadRequestIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement request = element.firstChildElement("request");
    d->fileName = request.attribute("filename");
    d->size = request.attribute("size").toLongLong();
    if (request.hasAttribute("content-type")) {
        QMimeDatabase mimeDb;
        QMimeType type = mimeDb.mimeTypeForName(request.attribute("content-type"));
        if (!type.isDefault() && type.isValid()) {
            d->contentType = type;
        }
    }
}

void QXmppHttpUploadRequestIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("request");
    writer->writeDefaultNamespace(ns_http_upload);
    // filename and size are required
    writer->writeAttribute("filename", d->fileName);
    writer->writeAttribute("size", QString::number(d->size));
    // content-type is optional
    if (!d->contentType.isDefault() && d->contentType.isValid()) {
        writer->writeAttribute("content-type", d->contentType.name());
    }
    writer->writeEndElement();
}
/// \endcond

class QXmppHttpUploadSlotIqPrivate : public QSharedData
{
public:
    QUrl putUrl;
    QUrl getUrl;
    QMap<QString, QString> putHeaders;
};

QXmppHttpUploadSlotIq::QXmppHttpUploadSlotIq()
    : d(new QXmppHttpUploadSlotIqPrivate())
{
}

/// Default copy-constructor
QXmppHttpUploadSlotIq::QXmppHttpUploadSlotIq(const QXmppHttpUploadSlotIq &) = default;
/// Default move-constructor
QXmppHttpUploadSlotIq::QXmppHttpUploadSlotIq(QXmppHttpUploadSlotIq &&) = default;
QXmppHttpUploadSlotIq::~QXmppHttpUploadSlotIq() = default;
/// Default assignment operator
QXmppHttpUploadSlotIq &QXmppHttpUploadSlotIq::operator=(const QXmppHttpUploadSlotIq &) = default;
/// Default move-assignment operator
QXmppHttpUploadSlotIq &QXmppHttpUploadSlotIq::operator=(QXmppHttpUploadSlotIq &&) = default;

///
/// Returns the URL for uploading via. HTTP PUT.
///
QUrl QXmppHttpUploadSlotIq::putUrl() const
{
    return d->putUrl;
}

///
/// Sets the URL the client should use for uploading.
///
void QXmppHttpUploadSlotIq::setPutUrl(const QUrl &putUrl)
{
    d->putUrl = putUrl;
}

///
/// Returns the URL to where the file will be served.
///
QUrl QXmppHttpUploadSlotIq::getUrl() const
{
    return d->getUrl;
}

///
/// Sets the download URL.
///
void QXmppHttpUploadSlotIq::setGetUrl(const QUrl &getUrl)
{
    d->getUrl = getUrl;
}

///
/// Returns a map of header fields (header name -> value) that need to be
/// included in the PUT (upload) request. This won't contain any other fields
/// than: "Authorization", "Cookie" or "Expires".
///
QMap<QString, QString> QXmppHttpUploadSlotIq::putHeaders() const
{
    return d->putHeaders;
}

///
/// Sets the header fields the client needs to include in the PUT (upload)
/// request. All fields other than "Authorization", "Cookie" or "Expires" will
/// be ignored.
///
void QXmppHttpUploadSlotIq::setPutHeaders(const QMap<QString, QString> &putHeaders)
{
    d->putHeaders.clear();
    std::for_each(putHeaders.keyBegin(), putHeaders.keyEnd(),
                  [this, &putHeaders](const QString &name) {
                      if (name == "Authorization" || name == "Cookie" || name == "Expires") {
                          d->putHeaders.insert(name, putHeaders[name]);
                      }
                  });
}

///
/// Returns true, if the the element is an HTTP File Upload slot result IQ.
///
bool QXmppHttpUploadSlotIq::isHttpUploadSlotIq(const QDomElement &element)
{
    if (element.tagName() == "iq") {
        QDomElement slot = element.firstChildElement("slot");
        return !slot.isNull() && slot.namespaceURI() == ns_http_upload;
    }
    return false;
}

/// \cond
void QXmppHttpUploadSlotIq::parseElementFromChild(const QDomElement &element)
{
    QDomElement slot = element.firstChildElement("slot");
    QDomElement put = slot.firstChildElement("put");
    d->getUrl = QUrl::fromEncoded(slot.firstChildElement("get").attribute("url").toUtf8());
    d->putUrl = QUrl::fromEncoded(put.attribute("url").toUtf8());
    if (put.hasChildNodes()) {
        QMap<QString, QString> headers;
        QDomElement header = put.firstChildElement("header");
        while (!header.isNull()) {
            headers[header.attribute("name")] = header.text();

            header = header.nextSiblingElement("header");
        }

        setPutHeaders(headers);
    }
}

void QXmppHttpUploadSlotIq::toXmlElementFromChild(QXmlStreamWriter *writer) const
{
    writer->writeStartElement("slot");
    writer->writeDefaultNamespace(ns_http_upload);

    writer->writeStartElement("put");
    writer->writeAttribute("url", d->putUrl.toEncoded());
    std::for_each(d->putHeaders.keyBegin(), d->putHeaders.keyEnd(), [=](const QString &name) {
        writer->writeStartElement("header");
        writer->writeAttribute("name", name);
        writer->writeCharacters(d->putHeaders.value(name));
        writer->writeEndElement();
    });
    writer->writeEndElement();

    writer->writeStartElement("get");
    writer->writeAttribute("url", d->getUrl.toEncoded());
    writer->writeEndElement();

    writer->writeEndElement();
}
/// \endcond
