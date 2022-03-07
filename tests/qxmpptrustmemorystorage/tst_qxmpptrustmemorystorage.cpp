// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "QXmppAtmTrustMemoryStorage.h"
#include "QXmppConstants.cpp"
#include "QXmppConstants_p.h"
#include "QXmppTrustMemoryStorage.h"
#include "QXmppTrustMessageKeyOwner.h"

#include "util.h"

class tst_QXmppTrustMemoryStorage : public QObject
{
    Q_OBJECT

private slots:
    // QXmppTrustMemoryStorage
    void testSecurityPolicy();
    void testOwnKeys();
    void testKeys();
    void testTrustLevels();
    void testResetAll();

    // QXmppAtmTrustMemoryStorage
    void atmTestKeysForPostponedTrustDecisions();
    void atmTestResetAll();

private:
    QXmppTrustMemoryStorage m_trustStorage;
    QXmppAtmTrustMemoryStorage m_atmTrustStorage;
};

void tst_QXmppTrustMemoryStorage::testSecurityPolicy()
{
    auto future = m_trustStorage.securityPolicy(ns_ox);
    QVERIFY(future.isFinished());
    auto result = future.result();
    QCOMPARE(result, QXmppTrustStorage::NoSecurityPolicy);

    m_trustStorage.setSecurityPolicy(ns_omemo, QXmppTrustStorage::Toakafa);

    future = m_trustStorage.securityPolicy(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::NoSecurityPolicy);

    future = m_trustStorage.securityPolicy(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::Toakafa);

    m_trustStorage.resetSecurityPolicy(ns_omemo);

    future = m_trustStorage.securityPolicy(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::NoSecurityPolicy);
}

void tst_QXmppTrustMemoryStorage::testOwnKeys()
{
    auto future = m_trustStorage.ownKey(ns_ox);
    QVERIFY(future.isFinished());
    auto result = future.result();
    QVERIFY(result.isEmpty());

    m_trustStorage.setOwnKey(ns_ox, QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));
    m_trustStorage.setOwnKey(ns_omemo, QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")));

    // own OX key
    future = m_trustStorage.ownKey(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));

    // own OMEMO key
    future = m_trustStorage.ownKey(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")));

    m_trustStorage.resetOwnKey(ns_omemo);

    // own OX key
    future = m_trustStorage.ownKey(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));

    // no own OMEMO key
    future = m_trustStorage.ownKey(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QVERIFY(result.isEmpty());
}

void tst_QXmppTrustMemoryStorage::testKeys()
{
    // no OMEMO keys
    auto future = m_trustStorage.keys(ns_omemo);
    QVERIFY(future.isFinished());
    auto result = future.result();
    QVERIFY(result.isEmpty());

    // no OMEMO keys (via JIDs)
    auto futureForJids = m_trustStorage.keys(ns_omemo,
                                             { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") });
    QVERIFY(futureForJids.isFinished());
    auto resultForJids = futureForJids.result();

    // no automatically trusted and authenticated OMEMO keys
    future = m_trustStorage.keys(ns_omemo,
                                 QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(future.isFinished());
    result = future.result();
    QVERIFY(result.isEmpty());

    // no automatically trusted and authenticated OMEMO key from Alice
    auto futureBool = m_trustStorage.hasKey(ns_omemo,
                                            QStringLiteral("alice@example.org"),
                                            QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureBool.isFinished());
    auto resultBool = futureBool.result();
    QVERIFY(!resultBool);

    // Store keys with the default trust level.
    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38=")),
          QByteArray::fromBase64(QByteArrayLiteral("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")) });

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")) },
        QXmppTrustStorage::ManuallyDistrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")),
          QByteArray::fromBase64(QByteArrayLiteral("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4=")) },
        QXmppTrustStorage::ManuallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")) },
        QXmppTrustStorage::Authenticated);

    m_trustStorage.addKeys(
        ns_ox,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
          QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")) },
        QXmppTrustStorage::Authenticated);

    QMultiHash<QString, QByteArray> automaticallyDistrustedKeys = { { QStringLiteral("alice@example.org"),
                                                                      QByteArray::fromBase64(QByteArrayLiteral("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38=")) },
                                                                    { QStringLiteral("alice@example.org"),
                                                                      QByteArray::fromBase64(QByteArrayLiteral("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")) } };
    QMultiHash<QString, QByteArray> manuallyDistrustedKeys = { { QStringLiteral("alice@example.org"),
                                                                 QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")) } };
    QMultiHash<QString, QByteArray> automaticallyTrustedKeys = { { QStringLiteral("alice@example.org"),
                                                                   QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")) },
                                                                 { QStringLiteral("bob@example.com"),
                                                                   QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")) } };
    QMultiHash<QString, QByteArray> manuallyTrustedKeys = { { QStringLiteral("bob@example.com"),
                                                              QByteArray::fromBase64(QByteArrayLiteral("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")) },
                                                            { QStringLiteral("bob@example.com"),
                                                              QByteArray::fromBase64(QByteArrayLiteral("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4=")) } };
    QMultiHash<QString, QByteArray> authenticatedKeys = { { QStringLiteral("bob@example.com"),
                                                            QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")) } };

    QHash<QByteArray, QXmppTrustStorage::TrustLevel> keysAlice = { { QByteArray::fromBase64(QByteArrayLiteral("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38=")),
                                                                     QXmppTrustStorage::AutomaticallyDistrusted },
                                                                   { QByteArray::fromBase64(QByteArrayLiteral("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")),
                                                                     QXmppTrustStorage::AutomaticallyDistrusted },
                                                                   { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
                                                                     QXmppTrustStorage::ManuallyDistrusted },
                                                                   { QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")),
                                                                     QXmppTrustStorage::AutomaticallyTrusted } };
    QHash<QByteArray, QXmppTrustStorage::TrustLevel> keysBob = { { QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
                                                                   QXmppTrustStorage::AutomaticallyTrusted },
                                                                 { QByteArray::fromBase64(QByteArrayLiteral("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")),
                                                                   QXmppTrustStorage::ManuallyTrusted },
                                                                 { QByteArray::fromBase64(QByteArrayLiteral("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4=")),
                                                                   QXmppTrustStorage::ManuallyTrusted },
                                                                 { QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")),
                                                                   QXmppTrustStorage::Authenticated } };

    // all OMEMO keys
    future = m_trustStorage.keys(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    QXmppTrustStorage::AutomaticallyDistrusted,
                    automaticallyDistrustedKeys),
                std::pair(
                    QXmppTrustStorage::ManuallyDistrusted,
                    manuallyDistrustedKeys),
                std::pair(
                    QXmppTrustStorage::AutomaticallyTrusted,
                    automaticallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::ManuallyTrusted,
                    manuallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::Authenticated,
                    authenticatedKeys) }));

    // automatically trusted and authenticated OMEMO keys
    future = m_trustStorage.keys(ns_omemo,
                                 QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    QXmppTrustStorage::AutomaticallyTrusted,
                    automaticallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::Authenticated,
                    authenticatedKeys) }));

    // all OMEMO keys (via JIDs)
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
                    QStringLiteral("alice@example.org"),
                    keysAlice),
                std::pair(
                    QStringLiteral("bob@example.com"),
                    keysBob) }));

    // Alice's OMEMO keys
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
            QStringLiteral("alice@example.org"),
            keysAlice) }));

    keysAlice = { { QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")),
                    QXmppTrustStorage::AutomaticallyTrusted } };
    keysBob = { { QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
                  QXmppTrustStorage::AutomaticallyTrusted },
                { QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")),
                  QXmppTrustStorage::Authenticated } };

    // automatically trusted and authenticated OMEMO keys (via JIDs)
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") },
                                        QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
                    QStringLiteral("alice@example.org"),
                    keysAlice),
                std::pair(
                    QStringLiteral("bob@example.com"),
                    keysBob) }));

    // Alice's automatically trusted and authenticated OMEMO keys
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org") },
                                        QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
            QStringLiteral("alice@example.org"),
            keysAlice) }));

    // at least one automatically trusted or authenticated OMEMO key from Alice
    futureBool = m_trustStorage.hasKey(ns_omemo,
                                       QStringLiteral("alice@example.org"),
                                       QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureBool.isFinished());
    resultBool = futureBool.result();
    QVERIFY(resultBool);

    m_trustStorage.removeKeys(ns_omemo,
                              { QByteArray::fromBase64(QByteArrayLiteral("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38=")),
                                QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")) });

    automaticallyDistrustedKeys = { { QStringLiteral("alice@example.org"),
                                      QByteArray::fromBase64(QByteArrayLiteral("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")) } };
    automaticallyTrustedKeys = { { QStringLiteral("bob@example.com"),
                                   QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")) } };

    keysAlice = { { QByteArray::fromBase64(QByteArrayLiteral("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")),
                    QXmppTrustStorage::AutomaticallyDistrusted },
                  { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
                    QXmppTrustStorage::ManuallyDistrusted } };
    keysBob = { { QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
                  QXmppTrustStorage::AutomaticallyTrusted },
                { QByteArray::fromBase64(QByteArrayLiteral("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")),
                  QXmppTrustStorage::ManuallyTrusted },
                { QByteArray::fromBase64(QByteArrayLiteral("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4=")),
                  QXmppTrustStorage::ManuallyTrusted },
                { QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")),
                  QXmppTrustStorage::Authenticated } };

    // OMEMO keys after removal
    future = m_trustStorage.keys(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    QXmppTrustStorage::AutomaticallyDistrusted,
                    automaticallyDistrustedKeys),
                std::pair(
                    QXmppTrustStorage::ManuallyDistrusted,
                    manuallyDistrustedKeys),
                std::pair(
                    QXmppTrustStorage::AutomaticallyTrusted,
                    automaticallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::ManuallyTrusted,
                    manuallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::Authenticated,
                    authenticatedKeys) }));

    // OMEMO keys after removal (via JIDs)
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
                    QStringLiteral("alice@example.org"),
                    keysAlice),
                std::pair(
                    QStringLiteral("bob@example.com"),
                    keysBob) }));

    // Alice's OMEMO keys after removal
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
            QStringLiteral("alice@example.org"),
            keysAlice) }));

    keysAlice = { { QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")),
                    QXmppTrustStorage::AutomaticallyTrusted } };
    keysBob = { { QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")),
                  QXmppTrustStorage::AutomaticallyTrusted },
                { QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")),
                  QXmppTrustStorage::Authenticated } };

    // automatically trusted and authenticated OMEMO keys after removal (via JIDs)
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") },
                                        QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
            QStringLiteral("bob@example.com"),
            keysBob) }));

    // Alice's automatically trusted and authenticated OMEMO keys after removal
    futureForJids = m_trustStorage.keys(ns_omemo,
                                        { QStringLiteral("alice@example.org") },
                                        QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QVERIFY(resultForJids.isEmpty());

    m_trustStorage.removeKeys(ns_omemo, QStringLiteral("alice@example.org"));

    // OMEMO keys after removing Alice's keys
    future = m_trustStorage.keys(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    QXmppTrustStorage::AutomaticallyTrusted,
                    automaticallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::ManuallyTrusted,
                    manuallyTrustedKeys),
                std::pair(
                    QXmppTrustStorage::Authenticated,
                    authenticatedKeys) }));

    m_trustStorage.removeKeys(ns_omemo);

    // no stored OMEMO keys
    future = m_trustStorage.keys(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QVERIFY(result.isEmpty());

    authenticatedKeys = { { QStringLiteral("alice@example.org"),
                            QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")) },
                          { QStringLiteral("alice@example.org"),
                            QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")) } };

    keysAlice = { { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
                    QXmppTrustStorage::Authenticated },
                  { QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")),
                    QXmppTrustStorage::Authenticated } };

    // remaining OX keys
    future = m_trustStorage.keys(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
            QXmppTrustStorage::Authenticated,
            authenticatedKeys) }));

    // remaining OX keys (via JIDs)
    futureForJids = m_trustStorage.keys(ns_ox,
                                        { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
            QStringLiteral("alice@example.org"),
            keysAlice) }));

    // Alice's remaining OX keys
    futureForJids = m_trustStorage.keys(ns_ox,
                                        { QStringLiteral("alice@example.org") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QCOMPARE(
        resultForJids,
        QHash({ std::pair(
            QStringLiteral("alice@example.org"),
            keysAlice) }));

    m_trustStorage.removeKeys(ns_ox);

    // no stored OX keys
    future = m_trustStorage.keys(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QVERIFY(result.isEmpty());

    // no stored OX keys (via JIDs)
    futureForJids = m_trustStorage.keys(ns_ox,
                                        { QStringLiteral("alice@example.org"), QStringLiteral("bob@example.com") });
    QVERIFY(futureForJids.isFinished());
    resultForJids = futureForJids.result();
    QVERIFY(resultForJids.isEmpty());

    // no automatically trusted or authenticated OX key from Alice
    futureBool = m_trustStorage.hasKey(ns_ox,
                                       QStringLiteral("alice@example.org"),
                                       QXmppTrustStorage::AutomaticallyTrusted | QXmppTrustStorage::Authenticated);
    QVERIFY(futureBool.isFinished());
    resultBool = futureBool.result();
    QVERIFY(!resultBool);
}

void tst_QXmppTrustMemoryStorage::testTrustLevels()
{
    m_trustStorage.addKeys(
        ns_ox,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")),
          QByteArray::fromBase64(QByteArrayLiteral("JU4pT7Ivpigtl+7QE87Bkq4r/C/mhI1FCjY5Wmjbtwg=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")) },
        QXmppTrustStorage::ManuallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    auto future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        QByteArray::fromBase64(QByteArrayLiteral("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")));
    QVERIFY(future.isFinished());
    auto result = future.result();
    QCOMPARE(result, QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.setTrustLevel(
        ns_omemo,
        { { QStringLiteral("alice@example.org"),
            QByteArray::fromBase64(QByteArrayLiteral("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")) },
          { QStringLiteral("bob@example.com"),
            QByteArray::fromBase64(QByteArrayLiteral("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA=")) } },
        QXmppTrustStorage::Authenticated);

    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        QByteArray::fromBase64(QByteArrayLiteral("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::Authenticated);

    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        QByteArray::fromBase64(QByteArrayLiteral("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::Authenticated);

    // Set the trust level of a key that is not stored yet.
    // It is added to the storage automatically.
    m_trustStorage.setTrustLevel(
        ns_omemo,
        { { QStringLiteral("alice@example.org"),
            QByteArray::fromBase64(QByteArrayLiteral("9w6oPjKyGSALd9gHq7sNOdOAkD5bHUVOKACNs89FjkA=")) } },
        QXmppTrustStorage::ManuallyTrusted);

    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        QByteArray::fromBase64(QByteArrayLiteral("9w6oPjKyGSALd9gHq7sNOdOAkD5bHUVOKACNs89FjkA=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::ManuallyTrusted);

    // Try to retrieve the trust level of a key that is not stored yet.
    // The default value is returned.
    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        QByteArray::fromBase64(QByteArrayLiteral("WXL4EDfzUGbVPQWjT9pmBeiCpCBzYZv3lUAaj+UbPyE=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::Undecided);

    // Set the trust levels of all authenticated keys belonging to Alice and
    // Bob.
    m_trustStorage.setTrustLevel(
        ns_omemo,
        { QStringLiteral("alice@example.org"),
          QStringLiteral("bob@example.com") },
        QXmppTrustStorage::Authenticated,
        QXmppTrustStorage::ManuallyDistrusted);

    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        QByteArray::fromBase64(QByteArrayLiteral("AZ/cF4OrUOILKO1gQBf62pQevOhBJ2NyHnXLwM4FDZU=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::ManuallyDistrusted);

    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        QByteArray::fromBase64(QByteArrayLiteral("9E51lG3vVmUn8CM7/AIcmIlLP2HPl6Ao0/VSf4VT/oA=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::ManuallyDistrusted);

    // Verify that the default trust level is returned for an unknown key.
    future = m_trustStorage.trustLevel(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        QByteArray::fromBase64(QByteArrayLiteral("wE06Gwf8f4DvDLFDoaCsGs8ibcUjf84WIOA2FAjPI3o=")));
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::Undecided);

    m_trustStorage.removeKeys(ns_ox);
    m_trustStorage.removeKeys(ns_omemo);
}

void tst_QXmppTrustMemoryStorage::testResetAll()
{
    m_trustStorage.setSecurityPolicy(ns_ox, QXmppTrustStorage::Toakafa);
    m_trustStorage.setSecurityPolicy(ns_omemo, QXmppTrustStorage::Toakafa);

    m_trustStorage.setOwnKey(ns_ox, QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));
    m_trustStorage.setOwnKey(ns_omemo, QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")));

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("WaAnpWyW1hnFooH3oJo9Ba5XYoksnLPeJRTAjxPbv38=")),
          QByteArray::fromBase64(QByteArrayLiteral("/1eK3R2LtjPBT3el8f0q4DvzqUJSfFy5fkKkKPNFNYw=")) });

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")) },
        QXmppTrustStorage::ManuallyDistrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("Ciemp4ZNzRJxnRD+k28vAie0kXJrwl4IrbfDy7n6OxE=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("rvSXBRd+EICMhQvVgcREQJxxP+T4EBmai4mYHBfJQGg=")) },
        QXmppTrustStorage::AutomaticallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("tCP1CI3pqSTVGzFYFyPYUMfMZ9Ck/msmfD0wH/VtJBM=")),
          QByteArray::fromBase64(QByteArrayLiteral("2fhJtrgoMJxfLI3084/YkYh9paqiSiLFDVL2m0qAgX4=")) },
        QXmppTrustStorage::ManuallyTrusted);

    m_trustStorage.addKeys(
        ns_omemo,
        QStringLiteral("bob@example.com"),
        { QByteArray::fromBase64(QByteArrayLiteral("YjVI04NcbTPvXLaA95RO84HPcSvyOgEZ2r5cTyUs0C8=")) },
        QXmppTrustStorage::Authenticated);

    m_trustStorage.addKeys(
        ns_ox,
        QStringLiteral("alice@example.org"),
        { QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")),
          QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")) },
        QXmppTrustStorage::Authenticated);

    m_trustStorage.resetAll(ns_omemo);

    auto future = m_trustStorage.securityPolicy(ns_omemo);
    QVERIFY(future.isFinished());
    auto result = future.result();
    QCOMPARE(result, QXmppTrustStorage::NoSecurityPolicy);

    future = m_trustStorage.securityPolicy(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(result, QXmppTrustStorage::Toakafa);

    auto futureKey = m_trustStorage.ownKey(ns_omemo);
    QVERIFY(futureKey.isFinished());
    auto resultKey = futureKey.result();
    QVERIFY(resultKey.isEmpty());

    futureKey = m_trustStorage.ownKey(ns_ox);
    QVERIFY(futureKey.isFinished());
    resultKey = futureKey.result();
    QCOMPARE(resultKey, QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")));

    auto futureKeys = m_trustStorage.keys(ns_omemo);
    QVERIFY(futureKeys.isFinished());
    auto resultKeys = futureKeys.result();
    QVERIFY(resultKeys.isEmpty());

    const QMultiHash<QString, QByteArray> authenticatedKeys = { { QStringLiteral("alice@example.org"),
                                                                  QByteArray::fromBase64(QByteArrayLiteral("aFABnX7Q/rbTgjBySYzrT2FsYCVYb49mbca5yB734KQ=")) },
                                                                { QStringLiteral("alice@example.org"),
                                                                  QByteArray::fromBase64(QByteArrayLiteral("IhpPjiKLchgrAG5cpSfTvdzPjZ5v6vTOluHEUehkgCA=")) } };

    futureKeys = m_trustStorage.keys(ns_ox);
    QVERIFY(futureKeys.isFinished());
    resultKeys = futureKeys.result();
    QCOMPARE(
        resultKeys,
        QHash({ std::pair(
            QXmppTrustStorage::Authenticated,
            authenticatedKeys) }));
}

void tst_QXmppTrustMemoryStorage::atmTestKeysForPostponedTrustDecisions()
{
    // The key 7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=
    // is set for both postponed authentication and distrusting.
    // Thus, it is only stored for postponed distrusting.
    QXmppTrustMessageKeyOwner keyOwnerAlice;
    keyOwnerAlice.setJid(QStringLiteral("alice@example.org"));
    keyOwnerAlice.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")),
                                   QByteArray::fromBase64(QByteArrayLiteral("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE=")),
                                   QByteArray::fromBase64(QByteArrayLiteral("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=")) });
    keyOwnerAlice.setDistrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")),
                                      QByteArray::fromBase64(QByteArrayLiteral("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=")) });

    QXmppTrustMessageKeyOwner keyOwnerBobTrustedKeys;
    keyOwnerBobTrustedKeys.setJid(QStringLiteral("bob@example.com"));
    keyOwnerBobTrustedKeys.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("GgTqeRLp1M+MEenzFQym2oqer9PfHukS4brJDQl5ARE=")) });

    m_atmTrustStorage.addKeysForPostponedTrustDecisions(ns_omemo,
                                                        QByteArray::fromBase64(QByteArrayLiteral("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
                                                        { keyOwnerAlice, keyOwnerBobTrustedKeys });

    QXmppTrustMessageKeyOwner keyOwnerBobDistrustedKeys;
    keyOwnerBobDistrustedKeys.setJid(QStringLiteral("bob@example.com"));
    keyOwnerBobDistrustedKeys.setDistrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA=")),
                                                  QByteArray::fromBase64(QByteArrayLiteral("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM=")) });

    m_atmTrustStorage.addKeysForPostponedTrustDecisions(ns_omemo,
                                                        QByteArray::fromBase64(QByteArrayLiteral("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
                                                        { keyOwnerBobDistrustedKeys });

    QXmppTrustMessageKeyOwner keyOwnerCarol;
    keyOwnerCarol.setJid(QStringLiteral("carol@example.net"));
    keyOwnerCarol.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU=")),
                                   QByteArray::fromBase64(QByteArrayLiteral("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE=")) });
    keyOwnerCarol.setDistrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g=")),
                                      QByteArray::fromBase64(QByteArrayLiteral("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g=")) });

    m_atmTrustStorage.addKeysForPostponedTrustDecisions(ns_ox,
                                                        QByteArray::fromBase64(QByteArrayLiteral("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
                                                        { keyOwnerCarol });

    QMultiHash<QString, QByteArray> trustedKeys = { { QStringLiteral("alice@example.org"),
                                                      QByteArray::fromBase64(QByteArrayLiteral("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")) },
                                                    { QStringLiteral("alice@example.org"),
                                                      QByteArray::fromBase64(QByteArrayLiteral("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE=")) },
                                                    { QStringLiteral("bob@example.com"),
                                                      QByteArray::fromBase64(QByteArrayLiteral("GgTqeRLp1M+MEenzFQym2oqer9PfHukS4brJDQl5ARE=")) } };
    QMultiHash<QString, QByteArray> distrustedKeys = { { QStringLiteral("alice@example.org"),
                                                         QByteArray::fromBase64(QByteArrayLiteral("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")) },
                                                       { QStringLiteral("alice@example.org"),
                                                         QByteArray::fromBase64(QByteArrayLiteral("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=")) } };

    auto future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo,
                                                                   { QByteArray::fromBase64(QByteArrayLiteral("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")) });
    QVERIFY(future.isFinished());
    auto result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    distrustedKeys = { { QStringLiteral("alice@example.org"),
                         QByteArray::fromBase64(QByteArrayLiteral("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")) },
                       { QStringLiteral("alice@example.org"),
                         QByteArray::fromBase64(QByteArrayLiteral("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=")) },
                       { QStringLiteral("bob@example.com"),
                         QByteArray::fromBase64(QByteArrayLiteral("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA=")) },
                       { QStringLiteral("bob@example.com"),
                         QByteArray::fromBase64(QByteArrayLiteral("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM=")) } };

    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo,
                                                              { QByteArray::fromBase64(QByteArrayLiteral("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
                                                                QByteArray::fromBase64(QByteArrayLiteral("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")) });
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    // Retrieve all keys.
    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    keyOwnerBobTrustedKeys.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA=")) });

    // Invert the trust in Bob's key
    // sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA= for the
    // sending endpoint with the key
    // IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=.
    m_atmTrustStorage.addKeysForPostponedTrustDecisions(
        ns_omemo,
        QByteArray::fromBase64(QByteArrayLiteral("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
        { keyOwnerBobTrustedKeys });

    trustedKeys = { { QStringLiteral("bob@example.com"),
                      QByteArray::fromBase64(QByteArrayLiteral("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA=")) } };
    distrustedKeys = { { QStringLiteral("bob@example.com"),
                         QByteArray::fromBase64(QByteArrayLiteral("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM=")) } };

    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo,
                                                              { QByteArray::fromBase64(QByteArrayLiteral("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")) });
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    m_atmTrustStorage.removeKeysForPostponedTrustDecisions(ns_omemo,
                                                           { QByteArray::fromBase64(QByteArrayLiteral("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")) });

    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    m_atmTrustStorage.addKeysForPostponedTrustDecisions(
        ns_omemo,
        QByteArray::fromBase64(QByteArrayLiteral("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
        { keyOwnerAlice });

    // The key QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE= is not removed
    // because its ID is passed within the parameter "keyIdsForDistrusting" but
    // stored for postponed authentication.
    m_atmTrustStorage.removeKeysForPostponedTrustDecisions(ns_omemo,
                                                           { QByteArray::fromBase64(QByteArrayLiteral("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")),
                                                             QByteArray::fromBase64(QByteArrayLiteral("sD6ilugEBeKxPsdDEyX43LSGKHKWd5MFEdhT+4RpsxA=")) },
                                                           { QByteArray::fromBase64(QByteArrayLiteral("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")),
                                                             QByteArray::fromBase64(QByteArrayLiteral("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE=")) });

    trustedKeys = { { QStringLiteral("alice@example.org"),
                      QByteArray::fromBase64(QByteArrayLiteral("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE=")) } };
    distrustedKeys = { { QStringLiteral("alice@example.org"),
                         QByteArray::fromBase64(QByteArrayLiteral("7y1t0LnmNBeXJka43XejFPLrKtQlSFATrYmy7xHaKYU=")) },
                       { QStringLiteral("bob@example.com"),
                         QByteArray::fromBase64(QByteArrayLiteral("X5tJ1D5rEeaeQE8eqhBKAj4KUZGYe3x+iHifaTBY1kM=")) } };

    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    // Remove all OMEMO keys.
    m_atmTrustStorage.removeKeysForPostponedTrustDecisions(ns_omemo);

    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo);
    QVERIFY(future.isFinished());
    result = future.result();
    QVERIFY(result.isEmpty());

    trustedKeys = { { QStringLiteral("carol@example.net"),
                      QByteArray::fromBase64(QByteArrayLiteral("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU=")) },
                    { QStringLiteral("carol@example.net"),
                      QByteArray::fromBase64(QByteArrayLiteral("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE=")) } };
    distrustedKeys = { { QStringLiteral("carol@example.net"),
                         QByteArray::fromBase64(QByteArrayLiteral("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g=")) },
                       { QStringLiteral("carol@example.net"),
                         QByteArray::fromBase64(QByteArrayLiteral("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g=")) } };

    // remaining OX keys
    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QCOMPARE(
        result,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));

    m_atmTrustStorage.removeKeysForPostponedTrustDecisions(ns_ox);

    // no OX keys
    future = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_ox);
    QVERIFY(future.isFinished());
    result = future.result();
    QVERIFY(result.isEmpty());
}

void tst_QXmppTrustMemoryStorage::atmTestResetAll()
{
    QXmppTrustMessageKeyOwner keyOwnerAlice;
    keyOwnerAlice.setJid(QStringLiteral("alice@example.org"));
    keyOwnerAlice.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("Wl53ZchbtAtCZQCHROiD20W7UnKTQgWQrjTHAVNw1ic=")),
                                   QByteArray::fromBase64(QByteArrayLiteral("QR05jrab7PFkSLhtdzyXrPfCqhkNCYCrlWATaBMTenE=")) });
    keyOwnerAlice.setDistrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("mB98hhdVps++skUuy4TGy/Vp6RQXLJO4JGf86FAUjyc=")) });

    QXmppTrustMessageKeyOwner keyOwnerBobTrustedKeys;
    keyOwnerBobTrustedKeys.setJid(QStringLiteral("bob@example.com"));
    keyOwnerBobTrustedKeys.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("GgTqeRLp1M+MEenzFQym2oqer9PfHukS4brJDQl5ARE=")) });

    m_atmTrustStorage.addKeysForPostponedTrustDecisions(ns_omemo,
                                                        QByteArray::fromBase64(QByteArrayLiteral("Mp6Y4wOF3aMcl38lb/VNbdPF9ucGFqSx2eyaEsqyHKE=")),
                                                        { keyOwnerAlice, keyOwnerBobTrustedKeys });

    QXmppTrustMessageKeyOwner keyOwnerCarol;
    keyOwnerCarol.setJid(QStringLiteral("carol@example.net"));
    keyOwnerCarol.setTrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU=")),
                                   QByteArray::fromBase64(QByteArrayLiteral("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE=")) });
    keyOwnerCarol.setDistrustedKeys({ QByteArray::fromBase64(QByteArrayLiteral("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g=")),
                                      QByteArray::fromBase64(QByteArrayLiteral("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g=")) });

    m_atmTrustStorage.addKeysForPostponedTrustDecisions(ns_ox,
                                                        QByteArray::fromBase64(QByteArrayLiteral("IL5iwDQwquH7yjb5RAiIP+nvYiBUsNCXtKB8IpKc9QU=")),
                                                        { keyOwnerCarol });

    m_atmTrustStorage.resetAll(ns_omemo);

    auto futureKeysForPostponedTrustDecisions = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_omemo);
    QVERIFY(futureKeysForPostponedTrustDecisions.isFinished());
    auto resultKeysForPostponedTrustDecisions = futureKeysForPostponedTrustDecisions.result();
    QVERIFY(resultKeysForPostponedTrustDecisions.isEmpty());

    QMultiHash<QString, QByteArray> trustedKeys = { { QStringLiteral("carol@example.net"),
                                                      QByteArray::fromBase64(QByteArrayLiteral("WcL+cEMpEeK+dpqg3Xd3amctzwP8h2MqwXcEzFf6LpU=")) },
                                                    { QStringLiteral("carol@example.net"),
                                                      QByteArray::fromBase64(QByteArrayLiteral("bH3R31z0N97K1fUwG3+bdBrVPuDfXguQapHudkfa5nE=")) } };
    QMultiHash<QString, QByteArray> distrustedKeys = { { QStringLiteral("carol@example.net"),
                                                         QByteArray::fromBase64(QByteArrayLiteral("N0B2StHKk1/slwg1rzybTFzjdg7FChc+3cXmTU/rS8g=")) },
                                                       { QStringLiteral("carol@example.net"),
                                                         QByteArray::fromBase64(QByteArrayLiteral("wsEN32UHCiNjYqTG/J63hY4Nu8tZT42Ni1FxrgyRQ5g=")) } };

    futureKeysForPostponedTrustDecisions = m_atmTrustStorage.keysForPostponedTrustDecisions(ns_ox);
    QVERIFY(futureKeysForPostponedTrustDecisions.isFinished());
    resultKeysForPostponedTrustDecisions = futureKeysForPostponedTrustDecisions.result();
    QCOMPARE(
        resultKeysForPostponedTrustDecisions,
        QHash({ std::pair(
                    true,
                    trustedKeys),
                std::pair(
                    false,
                    distrustedKeys) }));
}

QTEST_MAIN(tst_QXmppTrustMemoryStorage)
#include "tst_qxmpptrustmemorystorage.moc"
