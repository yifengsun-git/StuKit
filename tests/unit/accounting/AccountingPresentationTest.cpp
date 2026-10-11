#include "AccountingOverviewViewModel.h"
#include "MoneyFormatter.h"
#include "TransactionListModel.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QSignalSpy>
#include <QTest>

#include <limits>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QUuid &id)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = QStringLiteral("银行卡"),
        .openingBalance = Money::fromCents(0),
        .archived = false,
    });
    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(
    const QUuid &id, const QString &name, TransactionType type)
{
    const auto category = Category::create(CategoryData{
        .id = id,
        .name = name,
        .type = type,
        .archived = false,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

Transaction makeTransaction(
    TransactionType type,
    qint64 amountCents,
    const QString &utcTime,
    const QUuid &accountId,
    const QUuid &categoryId,
    const QString &note = {})
{
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = type,
        .amount = Money::fromCents(amountCents),
        .accountId = accountId,
        .categoryId = categoryId,
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

struct Fixture
{
    Fixture()
        : accountId(QUuid::createUuid())
        , incomeCategoryId(QUuid::createUuid())
        , expenseCategoryId(QUuid::createUuid())
        , useCase(transactions, accounts, categories)
    {
        const bool accountSaved = accounts.save(makeAccount(accountId));
        const bool incomeSaved = categories.save(makeCategory(
            incomeCategoryId, QStringLiteral("工资"), TransactionType::Income));
        const bool expenseSaved = categories.save(makeCategory(
            expenseCategoryId, QStringLiteral("餐饮"), TransactionType::Expense));
        Q_ASSERT(accountSaved && incomeSaved && expenseSaved);
        Q_UNUSED(accountSaved);
        Q_UNUSED(incomeSaved);
        Q_UNUSED(expenseSaved);
    }

    Transaction transaction(
        TransactionType type,
        qint64 amountCents,
        const QString &utcTime,
        const QString &note = {}) const
    {
        return makeTransaction(
            type,
            amountCents,
            utcTime,
            accountId,
            type == TransactionType::Income
                ? incomeCategoryId
                : expenseCategoryId,
            note);
    }

    InMemoryTransactionRepository transactions;
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    QUuid accountId;
    QUuid incomeCategoryId;
    QUuid expenseCategoryId;
    GetMonthlyOverview useCase;
};

} // namespace

class AccountingPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void formatsMoneyWithoutFloatingPoint();
    void exposesReadableTransactionRoles();
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

void AccountingPresentationTest::exposesReadableTransactionRoles()
{
    const QUuid accountId = QUuid::createUuid();
    const QUuid categoryId = QUuid::createUuid();
    const auto transaction = makeTransaction(
        TransactionType::Income,
        12345,
        QStringLiteral("2026-10-10T02:30:00.000Z"),
        accountId,
        categoryId,
        QStringLiteral("十月工资"));
    TransactionListModel model;
    model.replaceTransactions({MonthlyTransactionItem{
        .transaction = transaction,
        .accountName = QStringLiteral("银行卡"),
        .categoryName = QStringLiteral("工资"),
    }});

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0);
    QCOMPARE(model.data(index, TransactionListModel::IdRole).toString(),
             transaction.id().toString(QUuid::WithoutBraces));
    QVERIFY(model.data(index, TransactionListModel::IsIncomeRole).toBool());
    QCOMPARE(model.data(index, TransactionListModel::AmountTextRole).toString(),
             QStringLiteral("123.45"));
    QCOMPARE(model.data(index, TransactionListModel::AccountNameRole).toString(),
             QStringLiteral("银行卡"));
    QCOMPARE(model.data(index, TransactionListModel::CategoryNameRole).toString(),
             QStringLiteral("工资"));
    QCOMPARE(model.data(index, TransactionListModel::TypeTextRole).toString(),
             QStringLiteral("收入"));
    QCOMPARE(model.data(index, TransactionListModel::OccurredAtRole).toDateTime(),
             transaction.occurredAt());

    model.setTimeZone(utcPlusEight());
    QCOMPARE(model.data(
                 index, TransactionListModel::OccurredAtTextRole).toString(),
             QStringLiteral("10-10 10:30"));
    QCOMPARE(model.data(
                 index, TransactionListModel::FullOccurredAtTextRole).toString(),
             QStringLiteral("2026-10-10 10:30"));
    QCOMPARE(model.data(index, TransactionListModel::NoteRole).toString(),
             QStringLiteral("十月工资"));
    QCOMPARE(model.roleNames().value(TransactionListModel::CategoryNameRole),
             QByteArrayLiteral("categoryName"));
}

void AccountingPresentationTest::loadsMonthlyOverview()
{
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 7500,
        QStringLiteral("2026-10-10T02:00:00.000Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 8000,
        QStringLiteral("2026-10-11T02:00:00.000Z"))));
    AccountingOverviewViewModel viewModel(
        fixture.useCase, utcPlusEight(), QDate(2026, 10, 10));
    QSignalSpy overviewSpy(
        &viewModel, &AccountingOverviewViewModel::overviewChanged);

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
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 100,
        QStringLiteral("2026-10-10T00:00:00.000Z"))));
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Income, 200,
        QStringLiteral("2026-11-10T00:00:00.000Z"))));
    AccountingOverviewViewModel viewModel(
        fixture.useCase, QTimeZone::UTC, QDate(2026, 10, 1));
    QSignalSpy monthSpy(
        &viewModel, &AccountingOverviewViewModel::selectedMonthChanged);

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
    Fixture fixture;
    QVERIFY(fixture.transactions.save(fixture.transaction(
        TransactionType::Expense, 500,
        QStringLiteral("2026-09-30T16:00:00.000Z"))));
    AccountingOverviewViewModel viewModel(
        fixture.useCase, QTimeZone::UTC, QDate(2026, 10, 1));
    QSignalSpy timeZoneSpy(
        &viewModel, &AccountingOverviewViewModel::timeZoneChanged);

    QVERIFY(!viewModel.availableTimeZoneIds().isEmpty());
    QVERIFY(viewModel.availableTimeZoneIds().contains(QStringLiteral("UTC")));

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
    Fixture fixture;
    AccountingOverviewViewModel viewModel(
        fixture.useCase, QTimeZone::UTC, QDate(2026, 10, 1));

    QVERIFY(!viewModel.selectTimeZone(QStringLiteral("Not/A-Time-Zone")));
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("时区无效"));
    QCOMPARE(viewModel.timeZoneId(), QStringLiteral("UTC"));

    QVERIFY(!viewModel.selectFixedOffsetMinutes(15 * 60));
    QCOMPARE(viewModel.timeZoneId(), QStringLiteral("UTC"));
}

void AccountingPresentationTest::exposesAndClearsStorageError()
{
    Fixture fixture;
    fixture.transactions.setFindOccurredBetweenShouldSucceed(false);
    AccountingOverviewViewModel viewModel(
        fixture.useCase, QTimeZone::UTC, QDate(2026, 10, 1));
    QSignalSpy errorSpy(&viewModel, &AccountingOverviewViewModel::errorChanged);

    viewModel.load();
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法读取账单数据"));
    QVERIFY(viewModel.isEmpty());

    fixture.transactions.setFindOccurredBetweenShouldSucceed(true);
    viewModel.load();
    QVERIFY(!viewModel.hasError());
    QVERIFY(errorSpy.count() >= 2);
}

QTEST_APPLESS_MAIN(AccountingPresentationTest)

#include "AccountingPresentationTest.moc"
