#include "support/InMemoryTransactionRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Transaction makeTransaction(
    const QUuid &id,
    Money amount = Money::fromCents(1000),
    QDateTime occurredAt = QDateTime::fromString(
        QStringLiteral("2026-10-10T12:00:00Z"), Qt::ISODate))
{
    const auto transaction = Transaction::create(TransactionData{
        .id = id,
        .type = TransactionType::Expense,
        .amount = amount,
        .accountId = QUuid::createUuid(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = occurredAt,
        .note = QStringLiteral("测试"),
    });

    Q_ASSERT(transaction.has_value());
    return *transaction;
}

} // namespace

class TransactionRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void savesAndFindsTransaction();
    void returnsNulloptForMissingTransaction();
    void updatesTransactionWithSameId();
    void returnsAllTransactions();
    void findsTransactionsInHalfOpenRange();
    void returnsEmptyVectorForEmptyRange();
    void rejectsInvalidRange();
};

void TransactionRepositoryTest::savesAndFindsTransaction()
{
    InMemoryTransactionRepository repository;
    const auto id = QUuid::createUuid();

    QVERIFY(repository.save(makeTransaction(id)));

    const auto stored = repository.findById(id);
    QVERIFY(stored.has_value());
    QCOMPARE(stored->id(), id);
    QVERIFY(stored->amount() == Money::fromCents(1000));
}

void TransactionRepositoryTest::returnsNulloptForMissingTransaction()
{
    const InMemoryTransactionRepository repository;

    QVERIFY(!repository.findById(QUuid::createUuid()).has_value());
}

void TransactionRepositoryTest::updatesTransactionWithSameId()
{
    InMemoryTransactionRepository repository;
    const auto id = QUuid::createUuid();

    QVERIFY(repository.save(makeTransaction(id, Money::fromCents(1000))));
    QVERIFY(repository.save(makeTransaction(id, Money::fromCents(2500))));

    const auto transactions = repository.findAll();
    QVERIFY(transactions.has_value());
    QCOMPARE(transactions->size(), std::size_t{1});
    QVERIFY(transactions->front().amount() == Money::fromCents(2500));
}

void TransactionRepositoryTest::returnsAllTransactions()
{
    InMemoryTransactionRepository repository;

    QVERIFY(repository.save(makeTransaction(QUuid::createUuid())));
    QVERIFY(repository.save(makeTransaction(QUuid::createUuid())));

    QCOMPARE(repository.findAll()->size(), std::size_t{2});
}

void TransactionRepositoryTest::findsTransactionsInHalfOpenRange()
{
    InMemoryTransactionRepository repository;
    const auto start = QDateTime::fromString(
        QStringLiteral("2026-10-01T00:00:00.000Z"), Qt::ISODateWithMs);
    const auto end = QDateTime::fromString(
        QStringLiteral("2026-11-01T00:00:00.000Z"), Qt::ISODateWithMs);

    QVERIFY(repository.save(makeTransaction(
        QUuid::createUuid(), Money::fromCents(100), start.addMSecs(-1))));
    QVERIFY(repository.save(makeTransaction(
        QUuid::createUuid(), Money::fromCents(200), start)));
    QVERIFY(repository.save(makeTransaction(
        QUuid::createUuid(), Money::fromCents(300), end.addMSecs(-1))));
    QVERIFY(repository.save(makeTransaction(
        QUuid::createUuid(), Money::fromCents(400), end)));

    const auto result = repository.findOccurredBetween(start, end);
    QVERIFY(result.has_value());
    QCOMPARE(result->size(), std::size_t{2});
    QCOMPARE(result->front().amount(), Money::fromCents(300));
    QCOMPARE(result->back().amount(), Money::fromCents(200));
}

void TransactionRepositoryTest::returnsEmptyVectorForEmptyRange()
{
    const InMemoryTransactionRepository repository;
    const auto start = QDateTime::fromString(
        QStringLiteral("2026-10-01T00:00:00.000Z"), Qt::ISODateWithMs);
    const auto result = repository.findOccurredBetween(start, start.addMonths(1));

    QVERIFY(result.has_value());
    QVERIFY(result->empty());
}

void TransactionRepositoryTest::rejectsInvalidRange()
{
    const InMemoryTransactionRepository repository;
    const auto time = QDateTime::currentDateTimeUtc();

    QVERIFY(!repository.findOccurredBetween(time, time).has_value());
    QVERIFY(!repository.findOccurredBetween(time, time.addSecs(-1)).has_value());
    QVERIFY(!repository.findOccurredBetween(QDateTime{}, time).has_value());
}

QTEST_APPLESS_MAIN(TransactionRepositoryTest)

#include "TransactionRepositoryTest.moc"
