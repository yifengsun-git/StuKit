#include <QtTest>

#include "Transaction.h"

using stukit::accounting::Money;
using stukit::accounting::Transaction;
using stukit::accounting::TransactionData;
using stukit::accounting::TransactionType;

namespace {

TransactionData validTransactionData()
{
    TransactionData data;
    data.id = QUuid::fromString(
        QStringLiteral("{11111111-1111-1111-1111-111111111111}"));
    data.type = TransactionType::Expense;
    data.amount = Money::fromCents(1299);
    data.accountId = QUuid::fromString(
        QStringLiteral("{22222222-2222-2222-2222-222222222222}"));
    data.categoryId = QUuid::fromString(
        QStringLiteral("{33333333-3333-3333-3333-333333333333}"));
    data.occurredAt = QDateTime::fromString(
        QStringLiteral("2026-10-07T12:30:00Z"), Qt::ISODate);
    data.note = QStringLiteral("  lunch  ");
    return data;
}

} // namespace

class TransactionTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidTransaction();
    void createsIncomeTransaction();
    void rejectsNullTransactionId();
    void rejectsNullAccountId();
    void rejectsNullCategoryId();
    void rejectsNonPositiveAmount_data();
    void rejectsNonPositiveAmount();
    void rejectsInvalidOccurredAt();
};

void TransactionTest::createsValidTransaction()
{
    const TransactionData data = validTransactionData();
    const std::optional<Transaction> transaction = Transaction::create(data);

    QVERIFY(transaction.has_value());
    QCOMPARE(transaction->id(), data.id);
    QVERIFY(transaction->type() == TransactionType::Expense);
    QVERIFY(transaction->amount() == data.amount);
    QCOMPARE(transaction->accountId(), data.accountId);
    QCOMPARE(transaction->categoryId(), data.categoryId);
    QCOMPARE(transaction->occurredAt(), data.occurredAt);
    QCOMPARE(transaction->note(), QStringLiteral("lunch"));
}

void TransactionTest::createsIncomeTransaction()
{
    TransactionData data = validTransactionData();
    data.type = TransactionType::Income;

    const std::optional<Transaction> transaction = Transaction::create(data);

    QVERIFY(transaction.has_value());
    QVERIFY(transaction->type() == TransactionType::Income);
}

void TransactionTest::rejectsNullTransactionId()
{
    TransactionData data = validTransactionData();
    data.id = QUuid{};

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsNullAccountId()
{
    TransactionData data = validTransactionData();
    data.accountId = QUuid{};

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsNullCategoryId()
{
    TransactionData data = validTransactionData();
    data.categoryId = QUuid{};

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsNonPositiveAmount_data()
{
    QTest::addColumn<qint64>("cents");

    QTest::newRow("zero") << qint64{0};
    QTest::newRow("negative") << qint64{-1};
}

void TransactionTest::rejectsNonPositiveAmount()
{
    QFETCH(qint64, cents);

    TransactionData data = validTransactionData();
    data.amount = Money::fromCents(cents);

    QVERIFY(!Transaction::create(data).has_value());
}

void TransactionTest::rejectsInvalidOccurredAt()
{
    TransactionData data = validTransactionData();
    data.occurredAt = QDateTime{};

    QVERIFY(!Transaction::create(data).has_value());
}

QTEST_APPLESS_MAIN(TransactionTest)

#include "TransactionTest.moc"
