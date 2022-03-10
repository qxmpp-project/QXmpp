// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef QXMPPMIXINFOITEM_H
#define QXMPPMIXINFOITEM_H

#include "QXmppPubSubItem.h"

class QXmppMixInfoItemPrivate;

class QXMPP_EXPORT QXmppMixInfoItem : public QXmppPubSubItem
{
public:
    QXmppMixInfoItem();
    QXmppMixInfoItem(const QXmppMixInfoItem &);
    ~QXmppMixInfoItem();

    QXmppMixInfoItem &operator=(const QXmppMixInfoItem &);

    const QString &name() const;
    void setName(QString);

    const QString &description() const;
    void setDescription(QString);

    const QStringList &contactJids() const;
    void setContactJids(QStringList);

    static bool isItem(const QDomElement &itemElement);

protected:
    /// \cond
    void parsePayload(const QDomElement &payloadElement) override;
    void serializePayload(QXmlStreamWriter *writer) const override;
    /// \endcond

private:
    QSharedDataPointer<QXmppMixInfoItemPrivate> d;
};

#endif  // QXMPPMIXINFOITEM_H
