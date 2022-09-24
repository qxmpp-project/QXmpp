// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppFileEncryption.h"

#include "QcaInitializer_p.h"
#include <QtTest>

using namespace QXmpp;
using namespace QXmpp::Private;
using namespace QXmpp::Private::Encryption;

class tst_QXmppFileEncryption : public QObject
{
    Q_OBJECT

private:
    Q_SLOT void basic();
    Q_SLOT void deviceEncrypt();
    Q_SLOT void deviceDecrypt();
    Q_SLOT void paddingSize();
};

void tst_QXmppFileEncryption::basic()
{
    QcaInitializer encInit;

    QByteArray data =
        "This is an example text message";
    QByteArray key = "12345678901234567890123456789012";
    QByteArray iv = "data";

    auto encrypted = process(data, Aes256CbcPkcs7, Encode, key, iv);
    qDebug() << data.size() << "->" << encrypted.size();
    auto decrypted = process(encrypted, Aes256CbcPkcs7, Decode, key, iv);
    QCOMPARE(decrypted, data);
}

void tst_QXmppFileEncryption::deviceEncrypt()
{
    QcaInitializer encInit;

    QByteArray data =
        "v2qtI8tx5DxM6axUAZ+xsEwrtb0VYafAPlMWqpVMG+5PBE5wbZ7MZhDUEIdFkxchOIJqt";
    QByteArray key = "12345678901234567890123456789012";
    QByteArray iv = "12345678901234567890123456789012";

    auto buffer = std::make_unique<QBuffer>(&data);
    buffer->open(QIODevice::ReadOnly);

    EncryptionDevice encDev(std::move(buffer), Aes256CbcPkcs7, key, iv);

    auto encrypted = encDev.readAll();

    auto decrypted = process(encrypted, Aes256CbcPkcs7, Decode, key, iv);
    QCOMPARE(decrypted, data);
}

void tst_QXmppFileEncryption::deviceDecrypt()
{
    QcaInitializer encInit;

    QByteArray data =
        "v2qtI8tx5DxM6axUAZ+xsEwrtb0VYafAPlMWqpVMG+5PBE5wbZ7MZhDUEIdFkxchOIJqt";
    QByteArray key = "12345678901234567890123456789012";
    QByteArray iv = "12345678901234567890123456789012";

    auto encrypted = process(data, Aes256CbcPkcs7, Encode, key, iv);

    QByteArray decrypted;
    auto buffer = std::make_unique<QBuffer>(&decrypted);
    buffer->open(QIODevice::WriteOnly);

    DecryptionDevice decDev(std::move(buffer), Aes256CbcPkcs7, key, iv);
    decDev.write(encrypted);

    QCOMPARE(decrypted, data);
}

void tst_QXmppFileEncryption::paddingSize()
{
    constexpr auto MAX_BYTES_TEST = 1024;

    QcaInitializer encInit;

    QByteArray key = "12345678901234567890123456789012";
    QByteArray iv = "12345678901234567890123456789012";

    for (int i = 1; i <= MAX_BYTES_TEST; i++) {
        QByteArray data(i, 'a');
        auto buffer = std::make_unique<QBuffer>(&data);
        buffer->open(QIODevice::ReadOnly);

        EncryptionDevice encDev(std::move(buffer), Aes256CbcPkcs7, key, iv);
        auto reportedSize = encDev.size();
        auto encryptedData = encDev.readAll();

        QCOMPARE(reportedSize, encryptedData.size());

        auto decryptedData = process(encryptedData, Aes256CbcPkcs7, Decode, key, iv);
        QCOMPARE(decryptedData, data);
    }
}

QTEST_MAIN(tst_QXmppFileEncryption)
#include "tst_qxmppfileencryption.moc"
