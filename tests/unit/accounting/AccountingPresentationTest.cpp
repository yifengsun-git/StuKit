#include "AccountingOverviewViewModel.h"
#include "MoneyFormatter.h"
#include "TransactionListModel.h"
#include "support/InMemoryTransactionRepository.h"

#include <QSignalSpy>
#include <QTest>

#include <limits>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Transaction makeTransaction(TransactionType type, qint64 amountCents,
                            const QString &utcTime,
                            const QString &note = {})
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = QUuid::createUuid(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::fromString(utcTime, Qt::ISODateWithMs),
        .note = note,
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

QTimeZone utcPlusEight()
{
    return QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60);
}

} // namespace

class AccountingPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void formatsMoneyWithoutFloatingPoint();
    void exposesTransactionRoles();
    void loadsMonthlyOverview();
    void navigatesAndReloadsMonths();
    void appliesCustomFixedOffset();
    void rejectsInvalidTimeZone();
    void exposesAndClearsStorageError();
};

void AccountingPresentationTest::formatsMoneyWithoutFloatingPoint()
{
    QCOMPARE(formatMoney(Money::fromCents(0)), QStringLiteral("0.00"));
    QCOMPARE(formatMoney(Money::fromCents(12345)), QStringLiteral("123.45"));
    QCOMPARE(formatMoney(Money::fromCents(-5)), QStringLiteral("-0.05"));
    QCOMPARE(formatMoney(Money::fromCents(std::numeric_limits<qint64>::min())),
             QStringLiteral("-92233720368547758.08"));
}

void AccountingPresentationTest::exposesTransactionRoles()
{
    TransactionListModel model;
    const auto transaction = makeTransaction(
        TransactionType::Income, 12345,
        QStringLiteral("2026-10-10T02:30:00.000Z"),
        QStringLiteral("工资"));
    model.replaceTransactions({transaction});

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0);
    QCOMPARE(model.data(index, TransactionListModel::IdRole).toString(),
             transaction.id().toString(QUuid::WithoutBraces));
    QVERIFY(model.data(index, TransactionListModel::IsIncomeRole).toBool());
    QCOMPARE(model.data(index, TransactionListModel::AmountTextRole).toString(),
             QStringLiteral("123.45"));
    QCOMPARE(model.data(index, TransactionListModel::OccurredAtRole).toDateTime(),
             transaction.occurredAt());
    model.setTimeZone(utcPlusEight());
    QCOMPARE(model.data(index,
                        TransactionListModel::OccurredAtTextRole).toString(),
             QStringLiteral("10-10 10:30"));
    QCOMPARE(model.data(index, TransactionListModel::NoteRole).toString(),
             QStringLiteral("工资"));
    QCOMPARE(model.roleNames().value(TransactionListModel::AmountTextRole),
             QByteArrayLiteral("amountText"));
}

void AccountingPresentationTest::loadsMonthlyOverview()
{
    InMemoryTransactionRepository repository;
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 7500,
        QStringLiteral("2026-10-10T02:00:00.000Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 8000,
        QStringLiteral("2026-10-11T02:00:00.000Z"))));
    GetMonthlyOverview useCase(repository);
    AccountingOverviewViewModel viewModel(
        useCase, utcPlusEight(), QDate(2026, 10, 10));
    QSignalSpy overviewSpy(&viewModel,
                           &AccountingOverviewViewModel::overviewChanged);

    viewModel.load();

    QCOMPARE(overviewSpy.count(), 1);
    QCOMPARE(viewModel.monthLabel(), QStringLiteral("2026-10"));
    QCOMPARE(viewModel.totalIncomeText(), QStringLiteral("75.00"));
    QCOMPARE(viewModel.totalExpenseText(), QStringLiteral("80.00"));
    QCOMPARE(viewModel.balanceText(), QStringLiteral("-5.00"));
    QVERIFY(!viewModel.isEmpty());
    QVERIFY(!viewModel.hasError());
    QCOMPARE(viewModel.transactions()->rowCount(), 2);
}

void AccountingPresentationTest::navigatesAndReloadsMonths()
{
    InMemoryTransactionRepository repository;
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 100,
        QStringLiteral("2026-10-10T00:00:00.000Z"))));
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Income, 200,
        QStringLiteral("2026-11-10T00:00:00.000Z"))));
    GetMonthlyOverview useCase(repository);
    AccountingOverviewViewModel viewModel(
        useCase, QTimeZone::UTC, QDate(2026, 10, 1));
    QSignalSpy monthSpy(&viewModel,
                        &AccountingOverviewViewModel::selectedMonthChanged);

    viewModel.load();
    QCOMPARE(viewModel.totalIncomeText(), QStringLiteral("1.00"));

    viewModel.showNextMonth();
    QCOMPARE(monthSpy.count(), 1);
    QCOMPARE(viewModel.monthLabel(), QStringLiteral("2026-11"));
    QCOMPARE(viewModel.totalIncomeText(), QStringLiteral("2.00"));

    viewModel.showPreviousMonth();
    QCOMPARE(viewModel.monthLabel(), QStringLiteral("2026-10"));
    QCOMPARE(viewModel.totalIncomeText(), QStringLiteral("1.00"));
}

void AccountingPresentationTest::appliesCustomFixedOffset()
{
    InMemoryTransactionRepository repository;
    QVERIFY(repository.save(makeTransaction(
        TransactionType::Expense, 500,
        QStringLiteral("2026-09-30T16:00:00.000Z"))));
    GetMonthlyOverview useCase(repository);
    AccountingOverviewViewModel viewModel(
        useCase, QTimeZone::UTC, QDate(2026, 10, 1));
    QSignalSpy timeZoneSpy(&viewModel,
                           &AccountingOverviewViewModel::timeZoneChanged);

    viewModel.load();
    QVERIFY(viewModel.isEmpty());

    QVERIFY(viewModel.selectFixedOffsetMinutes(8 * 60));
    QCOMPARE(timeZoneSpy.count(), 1);
    QCOMPARE(viewModel.timeZoneId(), QStringLiteral("UTC+08:00"));
    QCOMPARE(viewModel.totalExpenseText(), QStringLiteral("5.00"));
    QCOMPARE(viewModel.transactions()->rowCount(), 1);
}

void AccountingPresentationTest::rejectsInvalidTimeZone()
{
    InMemoryTransactionRepository repository;
    GetMonthlyOverview useCase(repository);
    AccountingOverviewViewModel viewModel(
        useCase, QTimeZone::UTC, QDate(2026, 10, 1));

    QVERIFY(!viewModel.selectTimeZone(QStringLiteral("Not/A-Time-Zone")));
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("时区无效"));
    QCOMPARE(viewModel.timeZoneId(), QStringLiteral("UTC"));

    QVERIFY(!viewModel.selectFixedOffsetMinutes(15 * 60));
    QCOMPARE(viewModel.timeZoneId(), QStringLiteral("UTC"));
}

void AccountingPresentationTest::exposesAndClearsStorageError()
{
    InMemoryTransactionRepository repository;
    repository.setFindOccurredBetweenShouldSucceed(false);
    GetMonthlyOverview useCase(repository);
    AccountingOverviewViewModel viewModel(
        useCase, QTimeZone::UTC, QDate(2026, 10, 1));
    QSignalSpy errorSpy(&viewModel,
                        &AccountingOverviewViewModel::errorChanged);

    viewModel.load();
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法读取账单数据"));
    QVERIFY(viewModel.isEmpty());

    repository.setFindOccurredBetweenShouldSucceed(true);
    viewModel.load();
    QVERIFY(!viewModel.hasError());
    QVERIFY(errorSpy.count() >= 2);
}

QTEST_APPLESS_MAIN(AccountingPresentationTest)

#include "AccountingPresentationTest.moc"
