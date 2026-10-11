#include "DeleteTransaction.h"
#include "GetTransaction.h"
#include "UpdateTransaction.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

struct Fixture
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    UpdateTransaction update{transactions, accounts, categories};
    DeleteTransaction remove{transactions};
    GetTransaction get{transactions};
    QUuid accountId{QUuid::createUuid()};
    QUuid expenseCategoryId{QUuid::createUuid()};
    QUuid incomeCategoryId{QUuid::createUuid()};

    void seedReferences()
    {
        const auto account = Account::create(AccountData{
            .id = accountId,
            .name = QStringLiteral("银行卡"),
            .openingBalance = Money::fromCents(0),
            .archived = false,
        });
        const auto expense = Category::create(CategoryData{
            .id = expenseCategoryId,
            .name = QStringLiteral("餐饮"),
            .type = TransactionType::Expense,
            .archived = false,
        });
        const auto income = Category::create(CategoryData{
            .id = incomeCategoryId,
            .name = QStringLiteral("工资"),
            .type = TransactionType::Income,
            .archived = false,
        });
        QVERIFY(account.has_value());
        QVERIFY(expense.has_value());
        QVERIFY(income.has_value());
        QVERIFY(accounts.save(*account));
        QVERIFY(categories.save(*expense));
        QVERIFY(categories.save(*income));
    }

    Transaction seedTransaction(const QUuid &id)
    {
        const auto transaction = Transaction::create(TransactionData{
            .id = id,
            .type = TransactionType::Expense,
            .amount = Money::fromCents(1000),
            .accountId = accountId,
            .categoryId = expenseCategoryId,
            .occurredAt = QDateTime::fromString(
                QStringLiteral("2026-10-10T04:00:00Z"), Qt::ISODate),
            .note = QStringLiteral("原备注"),
        });
        Q_ASSERT(transaction.has_value());
        const bool saved = transactions.save(*transaction);
        Q_ASSERT(saved);
        Q_UNUSED(saved);
        return *transaction;
    }
};

UpdateTransactionRequest validUpdate(const Fixture &fixture, const QUuid &id)
{
    return UpdateTransactionRequest{
        .id = id,
        .type = TransactionType::Expense,
        .amount = Money::fromCents(2550),
        .accountId = fixture.accountId,
        .categoryId = fixture.expenseCategoryId,
        .occurredAt = QDateTime::fromString(
            QStringLiteral("2026-10-11T05:30:00Z"), Qt::ISODate),
        .note = QStringLiteral("修改后"),
    };
}

} // namespace

class ManageTransactionTest final : public QObject
{
    Q_OBJECT

private slots:
    void updatesExistingTransactionAndKeepsId();
    void doesNotUpsertMissingTransaction();
    void validatesCategoryTypeAndStorageFailure();
    void getsAndDeletesTransaction();
    void distinguishesDeleteFailures();
};

void ManageTransactionTest::updatesExistingTransactionAndKeepsId()
{
    Fixture fixture;
    fixture.seedReferences();
    const QUuid id = QUuid::createUuid();
    fixture.seedTransaction(id);

    const auto result = fixture.update.execute(validUpdate(fixture, id));

    QVERIFY(std::holds_alternative<Transaction>(result));
    const auto stored = fixture.transactions.findById(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->amount(), Money::fromCents(2550));
    QCOMPARE(stored->note(), QStringLiteral("修改后"));
    QCOMPARE(fixture.transactions.findAll().size(), std::size_t{1});
}

void ManageTransactionTest::doesNotUpsertMissingTransaction()
{
    Fixture fixture;
    fixture.seedReferences();

    const auto result = fixture.update.execute(
        validUpdate(fixture, QUuid::createUuid()));

    QVERIFY(std::holds_alternative<UpdateTransactionError>(result));
    QCOMPARE(std::get<UpdateTransactionError>(result),
             UpdateTransactionError::TransactionNotFound);
    QVERIFY(fixture.transactions.findAll().empty());
}

void ManageTransactionTest::validatesCategoryTypeAndStorageFailure()
{
    Fixture fixture;
    fixture.seedReferences();
    const QUuid id = QUuid::createUuid();
    fixture.seedTransaction(id);

    auto request = validUpdate(fixture, id);
    request.categoryId = fixture.incomeCategoryId;
    auto result = fixture.update.execute(request);
    QCOMPARE(std::get<UpdateTransactionError>(result),
             UpdateTransactionError::CategoryTypeMismatch);

    request.categoryId = fixture.expenseCategoryId;
    fixture.transactions.setSaveShouldSucceed(false);
    result = fixture.update.execute(request);
    QCOMPARE(std::get<UpdateTransactionError>(result),
             UpdateTransactionError::StorageFailure);
}

void ManageTransactionTest::getsAndDeletesTransaction()
{
    Fixture fixture;
    fixture.seedReferences();
    const QUuid id = QUuid::createUuid();
    fixture.seedTransaction(id);

    QVERIFY(std::holds_alternative<Transaction>(fixture.get.execute(id)));
    QVERIFY(!fixture.remove.execute(id).has_value());
    QVERIFY(!fixture.transactions.findById(id).has_value());
}

void ManageTransactionTest::distinguishesDeleteFailures()
{
    Fixture fixture;

    QCOMPARE(*fixture.remove.execute(QUuid{}),
             DeleteTransactionError::InvalidInput);
    QCOMPARE(*fixture.remove.execute(QUuid::createUuid()),
             DeleteTransactionError::TransactionNotFound);
    fixture.transactions.setRemoveShouldSucceed(false);
    QCOMPARE(*fixture.remove.execute(QUuid::createUuid()),
             DeleteTransactionError::StorageFailure);
}

QTEST_APPLESS_MAIN(ManageTransactionTest)

#include "ManageTransactionTest.moc"
