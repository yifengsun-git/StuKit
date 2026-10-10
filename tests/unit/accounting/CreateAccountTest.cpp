#include "CreateAccount.h"
#include "support/InMemoryAccountRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

class CreateAccountTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresValidAccount();
    void rejectsInvalidInputWithoutSaving();
    void reportsStorageFailure();
    void generatesDifferentIds();
};

void CreateAccountTest::createsAndStoresValidAccount()
{
    InMemoryAccountRepository repository;
    CreateAccount createAccount(repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("  现金  "),
        .openingBalance = Money::fromCents(12345),
    });

    QVERIFY(std::holds_alternative<Account>(result));
    const auto &account = std::get<Account>(result);
    QVERIFY(!account.id().isNull());
    QCOMPARE(account.name(), QStringLiteral("现金"));
    QVERIFY(account.openingBalance() == Money::fromCents(12345));
    QVERIFY(!account.isArchived());

    const auto stored = repository.findById(account.id());
    QVERIFY(stored.has_value());
    QCOMPARE(stored->name(), QStringLiteral("现金"));
}

void CreateAccountTest::rejectsInvalidInputWithoutSaving()
{
    InMemoryAccountRepository repository;
    CreateAccount createAccount(repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("   "),
        .openingBalance = Money::fromCents(0),
    });

    QVERIFY(std::holds_alternative<CreateAccountError>(result));
    QVERIFY(std::get<CreateAccountError>(result)
            == CreateAccountError::InvalidInput);
    QVERIFY(repository.findAll()->empty());
}

void CreateAccountTest::reportsStorageFailure()
{
    InMemoryAccountRepository repository;
    repository.setSaveShouldSucceed(false);
    CreateAccount createAccount(repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("现金"),
        .openingBalance = Money::fromCents(0),
    });

    QVERIFY(std::holds_alternative<CreateAccountError>(result));
    QVERIFY(std::get<CreateAccountError>(result)
            == CreateAccountError::StorageFailure);
    QVERIFY(repository.findAll()->empty());
}

void CreateAccountTest::generatesDifferentIds()
{
    InMemoryAccountRepository repository;
    CreateAccount createAccount(repository);

    const auto first = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("现金"),
    });
    const auto second = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("银行卡"),
    });

    QVERIFY(std::holds_alternative<Account>(first));
    QVERIFY(std::holds_alternative<Account>(second));
    QVERIFY(std::get<Account>(first).id() != std::get<Account>(second).id());
    QCOMPARE(repository.findAll()->size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(CreateAccountTest)

#include "CreateAccountTest.moc"
