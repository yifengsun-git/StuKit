#include "GetTransactionExportData.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QUuid &id, const QString &name, bool archived = false)
{
    const auto account = Account::create({
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });
    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(
    const QUuid &id,
    const QString &name,
    bool archived = false)
{
    const auto category = Category::create({
        .id = id,
        .name = name,
        .type = TransactionType::Expense,
        .archived = archived,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

Transaction makeTransaction(
    const QUuid &id,
    const QUuid &accountId,
    const QUuid &categoryId,
    const QString &utcTime)
{
    const auto transaction = Transaction::create({
        .id = id,
        .type = TransactionType::Expense,
        .amount = Money::fromCents(1234),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime::fromString(utcTime, Qt::ISODateWithMs),
        .note = QStringLiteral("测试"),
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

} // namespace

class GetTransactionExportDataTest final : public QObject
{
    Q_OBJECT

private slots:
    void attachesArchivedNamesAndSortsNewestFirst();
    void reportsRepositoryFailure();
    void reportsMissingRelation();
};

void GetTransactionExportDataTest::attachesArchivedNamesAndSortsNewestFirst()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const QUuid accountId = QUuid::createUuid();
    const QUuid categoryId = QUuid::createUuid();
    QVERIFY(accounts.save(makeAccount(
        accountId, QStringLiteral("已归档银行卡"), true)));
    QVERIFY(categories.save(makeCategory(
        categoryId, QStringLiteral("已归档餐饮"), true)));

    const QUuid olderId = QUuid::createUuid();
    const QUuid newerId = QUuid::createUuid();
    QVERIFY(transactions.save(makeTransaction(
        olderId, accountId, categoryId,
        QStringLiteral("2026-10-01T00:00:00.000Z"))));
    QVERIFY(transactions.save(makeTransaction(
        newerId, accountId, categoryId,
        QStringLiteral("2026-10-02T00:00:00.000Z"))));

    GetTransactionExportData useCase(transactions, accounts, categories);
    const auto result = useCase.execute();

    QVERIFY(std::holds_alternative<TransactionExportData>(result));
    const auto &data = std::get<TransactionExportData>(result);
    QCOMPARE(data.transactions.size(), std::size_t{2});
    QCOMPARE(data.transactions.front().transaction.id(), newerId);
    QCOMPARE(data.transactions.back().transaction.id(), olderId);
    QCOMPARE(data.transactions.front().accountName,
             QStringLiteral("已归档银行卡"));
    QCOMPARE(data.transactions.front().categoryName,
             QStringLiteral("已归档餐饮"));
}

void GetTransactionExportDataTest::reportsRepositoryFailure()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    transactions.setFindAllShouldSucceed(false);
    GetTransactionExportData useCase(transactions, accounts, categories);

    QCOMPARE(std::get<GetTransactionExportDataError>(useCase.execute()),
             GetTransactionExportDataError::StorageFailure);

    transactions.setFindAllShouldSucceed(true);
    accounts.setFindAllShouldSucceed(false);
    QCOMPARE(std::get<GetTransactionExportDataError>(useCase.execute()),
             GetTransactionExportDataError::StorageFailure);
}

void GetTransactionExportDataTest::reportsMissingRelation()
{
    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    QVERIFY(transactions.save(makeTransaction(
        QUuid::createUuid(), QUuid::createUuid(), QUuid::createUuid(),
        QStringLiteral("2026-10-01T00:00:00.000Z"))));
    GetTransactionExportData useCase(transactions, accounts, categories);

    QCOMPARE(std::get<GetTransactionExportDataError>(useCase.execute()),
             GetTransactionExportDataError::StorageFailure);
}

QTEST_APPLESS_MAIN(GetTransactionExportDataTest)

#include "GetTransactionExportDataTest.moc"
