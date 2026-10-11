#include "CreateTransaction.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteAccountRepository.h"
#include "repositories/SqliteCategoryRepository.h"
#include "repositories/SqliteTransactionRepository.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <memory>
#include <variant>

using namespace stukit::accounting;

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("stukit-transaction-repository-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

Account makeAccount(const QUuid &id, bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = QStringLiteral("现金"),
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });
    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(const QUuid &id, TransactionType type,
                      bool archived = false)
{
    const auto category = Category::create(CategoryData{
        .id = id,
        .name = type == TransactionType::Income ? QStringLiteral("工资")
                                                : QStringLiteral("餐饮"),
        .type = type,
        .archived = archived,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

Transaction makeTransaction(const QUuid &id, TransactionType type,
                            qint64 amountCents, const QUuid &accountId,
                            const QUuid &categoryId,
                            const QDateTime &occurredAt,
                            const QString &note = {})
{
    const auto transaction = Transaction::create(TransactionData{
        .id = id,
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = occurredAt,
        .note = note,
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

} // namespace

class SqliteTransactionRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsExpenseTransaction();
    void savesAndFindsIncomeTransaction();
    void returnsNulloptForMissingTransaction();
    void updatesTransactionWithSameId();
    void removesTransactionAndReportsMissingId();
    void returnsAllTransactionsNewestFirst();
    void findsTransactionsInHalfOpenTimeRange();
    void rejectsInvalidTimeRange();
    void rejectsMissingAccountForeignKey();
    void rejectsMissingCategoryForeignKey();
    void restrictsDeletingReferencedAccountAndCategory();
    void worksWithCreateTransactionUseCase();

private:
    void seedReferences(const QUuid &accountId, const QUuid &categoryId,
                        TransactionType type);

    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<SqliteDatabase> m_database;
    std::unique_ptr<SqliteAccountRepository> m_accounts;
    std::unique_ptr<SqliteCategoryRepository> m_categories;
    std::unique_ptr<SqliteTransactionRepository> m_transactions;
};

void SqliteTransactionRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-transaction-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());

    m_database = std::make_unique<SqliteDatabase>(uniqueConnectionName());
    QVERIFY2(m_database->open(
                 m_directory->filePath(QStringLiteral("stukit.sqlite"))),
             qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));

    m_accounts = std::make_unique<SqliteAccountRepository>(
        m_database->connection());
    m_categories = std::make_unique<SqliteCategoryRepository>(
        m_database->connection());
    m_transactions = std::make_unique<SqliteTransactionRepository>(
        m_database->connection());
}

void SqliteTransactionRepositoryTest::cleanup()
{
    m_transactions.reset();
    m_categories.reset();
    m_accounts.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteTransactionRepositoryTest::seedReferences(
    const QUuid &accountId, const QUuid &categoryId, TransactionType type)
{
    QVERIFY2(m_accounts->save(makeAccount(accountId)),
             qPrintable(m_accounts->lastError()));
    QVERIFY2(m_categories->save(makeCategory(categoryId, type)),
             qPrintable(m_categories->lastError()));
}

void SqliteTransactionRepositoryTest::savesAndFindsExpenseTransaction()
{
    const auto id = QUuid::createUuid();
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    const auto occurredAt = QDateTime::fromString(
        QStringLiteral("2026-10-01T08:30:15.123+08:00"), Qt::ISODateWithMs);
    seedReferences(accountId, categoryId, TransactionType::Expense);

    QVERIFY2(m_transactions->save(makeTransaction(
                 id, TransactionType::Expense, 12345, accountId, categoryId,
                 occurredAt, QStringLiteral(" 午餐 "))),
             qPrintable(m_transactions->lastError()));

    const auto stored = m_transactions->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_transactions->lastError()));
    QCOMPARE(stored->id(), id);
    QVERIFY(stored->type() == TransactionType::Expense);
    QCOMPARE(stored->amount(), Money::fromCents(12345));
    QCOMPARE(stored->accountId(), accountId);
    QCOMPARE(stored->categoryId(), categoryId);
    QCOMPARE(stored->occurredAt().toUTC(), occurredAt.toUTC());
    QCOMPARE(stored->note(), QStringLiteral("午餐"));

    QSqlQuery query(m_database->connection());
    query.prepare(QStringLiteral(
        "SELECT occurred_at FROM transactions WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(),
             QStringLiteral("2026-10-01T00:30:15.123Z"));
}

void SqliteTransactionRepositoryTest::savesAndFindsIncomeTransaction()
{
    const auto id = QUuid::createUuid();
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    seedReferences(accountId, categoryId, TransactionType::Income);

    QVERIFY(m_transactions->save(makeTransaction(
        id, TransactionType::Income, 800000, accountId, categoryId,
        QDateTime::fromString(QStringLiteral("2026-10-02T09:00:00.000Z"),
                              Qt::ISODateWithMs))));

    const auto stored = m_transactions->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_transactions->lastError()));
    QVERIFY(stored->type() == TransactionType::Income);
    QCOMPARE(stored->amount(), Money::fromCents(800000));
}

void SqliteTransactionRepositoryTest::returnsNulloptForMissingTransaction()
{
    const auto stored = m_transactions->findById(QUuid::createUuid());
    QVERIFY(!stored.has_value());
    QVERIFY(m_transactions->lastError().isEmpty());
}

void SqliteTransactionRepositoryTest::updatesTransactionWithSameId()
{
    const auto id = QUuid::createUuid();
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    const auto occurredAt = QDateTime::currentDateTimeUtc();
    seedReferences(accountId, categoryId, TransactionType::Expense);

    QVERIFY(m_transactions->save(makeTransaction(
        id, TransactionType::Expense, 100, accountId, categoryId, occurredAt)));
    QVERIFY(m_transactions->save(makeTransaction(
        id, TransactionType::Expense, 250, accountId, categoryId, occurredAt,
        QStringLiteral("更新后"))));

    const auto transactions = m_transactions->findAll();
    QVERIFY(transactions.has_value());
    QCOMPARE(transactions->size(), std::size_t{1});
    QCOMPARE(transactions->front().amount(), Money::fromCents(250));
    QCOMPARE(transactions->front().note(), QStringLiteral("更新后"));
}

void SqliteTransactionRepositoryTest::removesTransactionAndReportsMissingId()
{
    const auto id = QUuid::createUuid();
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    seedReferences(accountId, categoryId, TransactionType::Expense);
    QVERIFY(m_transactions->save(makeTransaction(
        id, TransactionType::Expense, 100, accountId, categoryId,
        QDateTime::currentDateTimeUtc())));

    QCOMPARE(m_transactions->removeById(id),
             RemoveTransactionResult::Removed);
    QVERIFY(!m_transactions->findById(id).has_value());
    QCOMPARE(m_transactions->removeById(id),
             RemoveTransactionResult::NotFound);
}

void SqliteTransactionRepositoryTest::returnsAllTransactionsNewestFirst()
{
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    seedReferences(accountId, categoryId, TransactionType::Expense);

    const auto older = QDateTime::fromString(
        QStringLiteral("2026-09-30T23:59:59.000Z"), Qt::ISODateWithMs);
    const auto newer = QDateTime::fromString(
        QStringLiteral("2026-10-01T00:00:00.000Z"), Qt::ISODateWithMs);
    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 100, accountId,
        categoryId, older)));
    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 200, accountId,
        categoryId, newer)));

    const auto transactions = m_transactions->findAll();
    QVERIFY2(m_transactions->lastError().isEmpty(),
             qPrintable(m_transactions->lastError()));
    QVERIFY(transactions.has_value());
    QCOMPARE(transactions->size(), std::size_t{2});
    QCOMPARE(transactions->front().amount(), Money::fromCents(200));
    QCOMPARE(transactions->back().amount(), Money::fromCents(100));
}

void SqliteTransactionRepositoryTest::findsTransactionsInHalfOpenTimeRange()
{
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    seedReferences(accountId, categoryId, TransactionType::Expense);
    const auto start = QDateTime::fromString(
        QStringLiteral("2026-10-01T00:00:00.000Z"), Qt::ISODateWithMs);
    const auto end = QDateTime::fromString(
        QStringLiteral("2026-11-01T00:00:00.000Z"), Qt::ISODateWithMs);

    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 100, accountId,
        categoryId, start.addMSecs(-1))));
    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 200, accountId,
        categoryId, start)));
    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 300, accountId,
        categoryId, end.addMSecs(-1))));
    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 400, accountId,
        categoryId, end)));

    const auto result = m_transactions->findOccurredBetween(start, end);
    QVERIFY2(result.has_value(), qPrintable(m_transactions->lastError()));
    QCOMPARE(result->size(), std::size_t{2});
    QCOMPARE(result->front().amount(), Money::fromCents(300));
    QCOMPARE(result->back().amount(), Money::fromCents(200));
}

void SqliteTransactionRepositoryTest::rejectsInvalidTimeRange()
{
    const auto time = QDateTime::currentDateTimeUtc();

    QVERIFY(!m_transactions->findOccurredBetween(time, time).has_value());
    QVERIFY(!m_transactions->lastError().isEmpty());
    QVERIFY(!m_transactions->findOccurredBetween(
        time, time.addSecs(-1)).has_value());
    QVERIFY(!m_transactions->lastError().isEmpty());
}

void SqliteTransactionRepositoryTest::rejectsMissingAccountForeignKey()
{
    const auto categoryId = QUuid::createUuid();
    QVERIFY(m_categories->save(
        makeCategory(categoryId, TransactionType::Expense)));

    QVERIFY(!m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 100,
        QUuid::createUuid(), categoryId, QDateTime::currentDateTimeUtc())));
    QVERIFY(!m_transactions->lastError().isEmpty());
}

void SqliteTransactionRepositoryTest::rejectsMissingCategoryForeignKey()
{
    const auto accountId = QUuid::createUuid();
    QVERIFY(m_accounts->save(makeAccount(accountId)));

    QVERIFY(!m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 100, accountId,
        QUuid::createUuid(), QDateTime::currentDateTimeUtc())));
    QVERIFY(!m_transactions->lastError().isEmpty());
}

void SqliteTransactionRepositoryTest::restrictsDeletingReferencedAccountAndCategory()
{
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    seedReferences(accountId, categoryId, TransactionType::Expense);
    QVERIFY(m_transactions->save(makeTransaction(
        QUuid::createUuid(), TransactionType::Expense, 100, accountId,
        categoryId, QDateTime::currentDateTimeUtc())));

    {
        QSqlQuery query(m_database->connection());
        query.prepare(QStringLiteral("DELETE FROM accounts WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"),
                        accountId.toString(QUuid::WithoutBraces));
        QVERIFY(!query.exec());
    }

    {
        QSqlQuery query(m_database->connection());
        query.prepare(QStringLiteral("DELETE FROM categories WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"),
                        categoryId.toString(QUuid::WithoutBraces));
        QVERIFY(!query.exec());
    }
}

void SqliteTransactionRepositoryTest::worksWithCreateTransactionUseCase()
{
    const auto accountId = QUuid::createUuid();
    const auto categoryId = QUuid::createUuid();
    seedReferences(accountId, categoryId, TransactionType::Expense);
    CreateTransaction createTransaction(
        *m_transactions, *m_accounts, *m_categories);

    const auto result = createTransaction.execute(CreateTransactionRequest{
        .type = TransactionType::Expense,
        .amount = Money::fromCents(1999),
        .accountId = accountId,
        .categoryId = categoryId,
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .note = QStringLiteral("晚餐"),
    });

    QVERIFY(std::holds_alternative<Transaction>(result));
    const auto &created = std::get<Transaction>(result);
    const auto stored = m_transactions->findById(created.id());
    QVERIFY2(stored.has_value(), qPrintable(m_transactions->lastError()));
    QCOMPARE(stored->amount(), Money::fromCents(1999));
    QCOMPARE(stored->note(), QStringLiteral("晚餐"));
}

QTEST_GUILESS_MAIN(SqliteTransactionRepositoryTest)

#include "SqliteTransactionRepositoryTest.moc"
