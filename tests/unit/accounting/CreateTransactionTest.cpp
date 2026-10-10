#include "CreateTransaction.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QUuid &id, bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = QStringLiteral("日常账户"),
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });

    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(
    const QUuid &id,
    TransactionType type = TransactionType::Expense,
    bool archived = false)
{
    const auto category = Category::create(CategoryData{
        .id = id,
        .name = QStringLiteral("餐饮"),
        .type = type,
        .archived = archived,
    });

    Q_ASSERT(category.has_value());
    return *category;
}

CreateTransactionRequest validRequest(
    const QUuid &accountId,
    const QUuid &categoryId)
{
    return CreateTransactionRequest{
        .type = TransactionType::Expense,
        .amount = Money::fromCents(2599),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime::fromString(
            QStringLiteral("2026-10-10T12:00:00Z"), Qt::ISODate),
        .note = QStringLiteral("  午餐  "),
    };
}

} // namespace

class CreateTransactionTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndStoresValidTransaction();
    void rejectsInvalidInputWithoutSaving();
    void reportsMissingAccount();
    void rejectsArchivedAccount();
    void reportsMissingCategory();
    void rejectsArchivedCategory();
    void rejectsCategoryTypeMismatch();
    void reportsStorageFailure();
};

void CreateTransactionTest::createsAndStoresValidTransaction()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(accountId)));
    QVERIFY(categories.save(makeCategory(categoryId)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto request = validRequest(accountId, categoryId);
    const auto result = createTransaction.execute(request);

    QVERIFY(std::holds_alternative<Transaction>(result));
    const auto &created = std::get<Transaction>(result);
    QVERIFY(!created.id().isNull());
    QVERIFY(created.type() == TransactionType::Expense);
    QVERIFY(created.amount() == Money::fromCents(2599));
    QCOMPARE(created.accountId(), accountId);
    QCOMPARE(created.categoryId(), categoryId);
    QCOMPARE(created.occurredAt(), request.occurredAt);
    QCOMPARE(created.note(), QStringLiteral("午餐"));

    const auto stored = transactions.findById(created.id());
    QVERIFY(stored.has_value());
    QVERIFY(stored->amount() == Money::fromCents(2599));
}

void CreateTransactionTest::rejectsInvalidInputWithoutSaving()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    CreateTransaction createTransaction(transactions, accounts, categories);
    auto request = validRequest(QUuid::createUuid(), QUuid::createUuid());
    request.amount = Money::fromCents(0);

    const auto result = createTransaction.execute(request);

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::InvalidInput);
    QVERIFY(transactions.findAll().empty());
}

void CreateTransactionTest::reportsMissingAccount()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(categories.save(makeCategory(categoryId)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto result = createTransaction.execute(
        validRequest(accountId, categoryId));

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::AccountNotFound);
    QVERIFY(transactions.findAll().empty());
}

void CreateTransactionTest::rejectsArchivedAccount()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(accountId, true)));
    QVERIFY(categories.save(makeCategory(categoryId)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto result = createTransaction.execute(
        validRequest(accountId, categoryId));

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::AccountArchived);
    QVERIFY(transactions.findAll().empty());
}

void CreateTransactionTest::reportsMissingCategory()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(accountId)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto result = createTransaction.execute(
        validRequest(accountId, categoryId));

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::CategoryNotFound);
    QVERIFY(transactions.findAll().empty());
}

void CreateTransactionTest::rejectsArchivedCategory()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(accountId)));
    QVERIFY(categories.save(makeCategory(
        categoryId, TransactionType::Expense, true)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto result = createTransaction.execute(
        validRequest(accountId, categoryId));

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::CategoryArchived);
    QVERIFY(transactions.findAll().empty());
}

void CreateTransactionTest::rejectsCategoryTypeMismatch()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(accountId)));
    QVERIFY(categories.save(makeCategory(
        categoryId, TransactionType::Income)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto result = createTransaction.execute(
        validRequest(accountId, categoryId));

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::CategoryTypeMismatch);
    QVERIFY(transactions.findAll().empty());
}

void CreateTransactionTest::reportsStorageFailure()
{
    InMemoryTransactionRepository transactions;
    transactions.setSaveShouldSucceed(false);
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(accountId)));
    QVERIFY(categories.save(makeCategory(categoryId)));
    CreateTransaction createTransaction(transactions, accounts, categories);

    const auto result = createTransaction.execute(
        validRequest(accountId, categoryId));

    QVERIFY(std::holds_alternative<CreateTransactionError>(result));
    QVERIFY(std::get<CreateTransactionError>(result)
            == CreateTransactionError::StorageFailure);
    QVERIFY(transactions.findAll().empty());
}

QTEST_APPLESS_MAIN(CreateTransactionTest)

#include "CreateTransactionTest.moc"
