#include "support/InMemoryTransactionRepository.h"

#include <QTest>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Transaction makeTransaction(
    const QUuid &id,
    Money amount = Money::fromCents(1000))
{
    const auto transaction = Transaction::create(TransactionData{
        .id = id,
        .type = TransactionType::Expense,
        .amount = amount,
        .accountId = QUuid::createUuid(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::fromString(
            QStringLiteral("2026-10-10T12:00:00Z"), Qt::ISODate),
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
    QCOMPARE(transactions.size(), std::size_t{1});
    QVERIFY(transactions.front().amount() == Money::fromCents(2500));
}

void TransactionRepositoryTest::returnsAllTransactions()
{
    InMemoryTransactionRepository repository;

    QVERIFY(repository.save(makeTransaction(QUuid::createUuid())));
    QVERIFY(repository.save(makeTransaction(QUuid::createUuid())));

    QCOMPARE(repository.findAll().size(), std::size_t{2});
}

QTEST_APPLESS_MAIN(TransactionRepositoryTest)

#include "TransactionRepositoryTest.moc"
