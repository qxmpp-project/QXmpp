// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPFILEMETADATA_H
#define QXMPPFILEMETADATA_H

#include "QXmppGlobal.h"
#include "QXmppHash.h"

#include <optional>

#include <QMimeType>
#include <QSharedDataPointer>

class QDomElement;
class QDateTime;
class QXmlStreamWriter;
class QXmppThumbnail;
class QXmppFileMetadataPrivate;

class QXMPP_EXPORT QXmppFileMetadata
{
public:
    QXmppFileMetadata();

    QXMPP_PRIVATE_DECLARE_RULE_OF_SIX(QXmppFileMetadata)

    /// \cond
    bool parse(const QDomElement &el);
    void toXml(QXmlStreamWriter *writer) const;
    /// \endcond

    const std::optional<QDateTime> &lastModified() const;
    void setLastModified(const std::optional<QDateTime> &date);

    const std::optional<QString> &description() const;
    void setDescription(const std::optional<QString> &description);

    const QVector<QXmppHash> &hashes() const;
    void setHashes(const QVector<QXmppHash> &hashes);

    std::optional<uint32_t> height() const;
    void setHeight(std::optional<uint32_t> height);

    std::optional<uint32_t> length() const;
    void setLength(std::optional<uint32_t> length);

    const std::optional<QMimeType> &mediaType() const;
    void setMediaType(std::optional<QMimeType> mediaType);

    std::optional<QString> filename() const;
    void setFilename(std::optional<QString>);

    std::optional<uint64_t> size() const;
    void setSize(std::optional<uint64_t> size);

    const std::optional<QXmppThumbnail> &thumbnail() const;
    void setThumbnail(const std::optional<QXmppThumbnail> &thumbnail);

    std::optional<uint32_t> width() const;
    void setWidth(std::optional<uint32_t> width);

private:
    QSharedDataPointer<QXmppFileMetadataPrivate> d;
};

#endif
