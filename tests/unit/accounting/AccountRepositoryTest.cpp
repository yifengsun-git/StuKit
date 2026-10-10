#include "support/InMemoryAccountRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account createAccount(const QUuid &id, const QString &name)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = false,
    });

    Q_ASSERT(account.has_value());
    return *account;
}

} // namespace

class AccountRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void savesAndFindsAccount();
    void returnsNulloptForMissingAccount();
    void updatesAccountWithSameId();
    void returnsAllAccounts();
};

void AccountRepositoryTest::savesAndFindsAccount()
{
    InMemoryAccountRepository repository;
    const auto id = QUuid::createUuid();
    const auto account = createAccount(id, QStringLiteral("现金"));

    QVERIFY(repository.save(account));

    const auto stored = repository.findById(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("现金"));
}

void AccountRepositoryTest::returnsNulloptForMissingAccount()
{
    const InMemoryAccountRepository repository;

    const auto stored = repository.findById(QUuid::createUuid());

    QVERIFY(!stored.has_value());
}

void AccountRepositoryTest::updatesAccountWithSameId()
{
    InMemoryAccountRepository repository;
    const auto id = QUuid::createUuid();

    QVERIFY(repository.save(createAccount(id, QStringLiteral("现金"))));
    QVERIFY(repository.save(createAccount(id, QStringLiteral("日常账户"))));

    const auto accounts = repository.findAll();
    QVERIFY(accounts.has_value());
    QCOMPARE(accounts->size(), std::size_t{1});
    QCOMPARE(accounts->front().name(), QStringLiteral("日常账户"));
}

void AccountRepositoryTest::returnsAllAccounts()
{
    InMemoryAccountRepository repository;

    QVERIFY(repository.save(
        createAccount(QUuid::createUuid(), QStringLiteral("现金"))));
    QVERIFY(repository.save(
        createAccount(QUuid::createUuid(), QStringLiteral("银行卡"))));

    const auto accounts = repository.findAll();

    QVERIFY(accounts.has_value());
    QCOMPARE(accounts->size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(AccountRepositoryTest)

#include "AccountRepositoryTest.moc"
