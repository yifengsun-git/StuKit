#include "CategoryListModel.h"
#include "CreateTransactionViewModel.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>
#include <QUuid>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QString &name, bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = name,
        .openingBalance = Money::fromCents(0),
        .archived = archived,
    });
    Q_ASSERT(account.has_value());
    return *account;
}

Category makeCategory(
    const QString &name, TransactionType type, bool archived = false)
{
    const auto category = Category::create(CategoryData{
        .id = QUuid::createUuid(),
        .name = name,
        .type = type,
        .archived = archived,
    });
    Q_ASSERT(category.has_value());
    return *category;
}

struct Fixture
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    CreateTransaction createTransaction{transactions, accounts, categories};
    GetAccountingSetup getSetup{accounts, categories};
    CreateTransactionViewModel viewModel{
        createTransaction,
        getSetup,
        QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60)};
};

void addCompleteSetup(Fixture &fixture)
{
    QVERIFY(fixture.accounts.save(makeAccount(QStringLiteral("银行卡"))));
    QVERIFY(fixture.categories.save(makeCategory(
        QStringLiteral("餐饮"), TransactionType::Expense)));
    QVERIFY(fixture.categories.save(makeCategory(
        QStringLiteral("工资"), TransactionType::Income)));
}

void fillValidExpenseForm(Fixture &fixture)
{
    fixture.viewModel.resetForNewTransaction();
    fixture.viewModel.setAmountText(QStringLiteral("25.50"));
    fixture.viewModel.setOccurredDateText(QStringLiteral("2026-10-10"));
    fixture.viewModel.setOccurredTimeText(QStringLiteral("12:34"));
    fixture.viewModel.setNote(QStringLiteral("  午饭  "));
}

} // namespace

class CreateTransactionPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void filtersArchivedAndMismatchedOptions();
    void createsTransactionInCustomTimeZone();
    void rejectsInvalidInputAndPreservesForm();
    void rejectsDaylightSavingGap();
    void reportsSetupQueryFailure();
};

void CreateTransactionPresentationTest::filtersArchivedAndMismatchedOptions()
{
    Fixture fixture;
    addCompleteSetup(fixture);
    QVERIFY(fixture.accounts.save(makeAccount(QStringLiteral("旧账户"), true)));
    QVERIFY(fixture.categories.save(makeCategory(
        QStringLiteral("旧支出"), TransactionType::Expense, true)));

    fixture.viewModel.resetForNewTransaction();

    QVERIFY(fixture.viewModel.isReady());
    QVERIFY(!fixture.viewModel.isIncome());
    QCOMPARE(fixture.viewModel.availableAccounts()->rowCount(), 1);
    QCOMPARE(fixture.viewModel.availableCategories()->rowCount(), 1);
    QVERIFY(!fixture.viewModel.selectedAccountId().isEmpty());
    QVERIFY(!fixture.viewModel.selectedCategoryId().isEmpty());

    fixture.viewModel.setIncome(true);
    QCOMPARE(fixture.viewModel.availableCategories()->rowCount(), 1);
    const QModelIndex index = fixture.viewModel.availableCategories()->index(0, 0);
    QCOMPARE(fixture.viewModel.availableCategories()->data(
                 index, CategoryListModel::IsIncomeRole),
             QVariant(true));
}

void CreateTransactionPresentationTest::createsTransactionInCustomTimeZone()
{
    Fixture fixture;
    addCompleteSetup(fixture);
    fillValidExpenseForm(fixture);
    QSignalSpy createdSpy(
        &fixture.viewModel,
        &CreateTransactionViewModel::transactionCreated);

    QVERIFY(fixture.viewModel.submit());
    QCOMPARE(createdSpy.count(), 1);
    QVERIFY(!fixture.viewModel.hasError());
    QVERIFY(fixture.viewModel.amountText().isEmpty());
    QVERIFY(fixture.viewModel.note().isEmpty());

    const auto stored = fixture.transactions.findAll();
    QCOMPARE(stored.size(), std::size_t{1});
    QCOMPARE(stored.front().amount().cents(), qint64{2550});
    QCOMPARE(stored.front().note(), QStringLiteral("午饭"));
    QCOMPARE(stored.front().occurredAt().toUTC(),
             QDateTime(QDate(2026, 10, 10), QTime(4, 34), Qt::UTC));
}

void CreateTransactionPresentationTest::rejectsInvalidInputAndPreservesForm()
{
    Fixture fixture;
    addCompleteSetup(fixture);
    fillValidExpenseForm(fixture);

    fixture.viewModel.setAmountText(QStringLiteral("1.234"));
    QVERIFY(!fixture.viewModel.submit());
    QCOMPARE(fixture.viewModel.errorMessage(), QStringLiteral("金额格式无效"));
    QCOMPARE(fixture.viewModel.amountText(), QStringLiteral("1.234"));

    fixture.viewModel.setAmountText(QStringLiteral("0"));
    QVERIFY(!fixture.viewModel.submit());
    QCOMPARE(fixture.viewModel.errorMessage(), QStringLiteral("金额必须大于 0"));

    fixture.viewModel.setAmountText(QStringLiteral("25.50"));
    fixture.viewModel.setOccurredDateText(QStringLiteral("2026-02-30"));
    QVERIFY(!fixture.viewModel.submit());
    QCOMPARE(fixture.viewModel.errorMessage(),
             QStringLiteral("发生日期、时间或时区无效"));

    fixture.viewModel.setOccurredDateText(QStringLiteral("2026-10-10"));
    fixture.transactions.setSaveShouldSucceed(false);
    QVERIFY(!fixture.viewModel.submit());
    QCOMPARE(fixture.viewModel.errorMessage(), QStringLiteral("无法保存账单"));
    QCOMPARE(fixture.viewModel.amountText(), QStringLiteral("25.50"));
    QCOMPARE(fixture.viewModel.note(), QStringLiteral("  午饭  "));
}

void CreateTransactionPresentationTest::rejectsDaylightSavingGap()
{
    const QTimeZone newYork(QByteArrayLiteral("America/New_York"));
    if (!newYork.isValid()) {
        QSKIP("America/New_York time zone is unavailable");
    }

    Fixture fixture;
    addCompleteSetup(fixture);
    fixture.viewModel.setTimeZone(newYork);
    fixture.viewModel.resetForNewTransaction();
    fixture.viewModel.setAmountText(QStringLiteral("10"));
    fixture.viewModel.setOccurredDateText(QStringLiteral("2026-03-08"));
    fixture.viewModel.setOccurredTimeText(QStringLiteral("02:30"));

    QVERIFY(!fixture.viewModel.submit());
    QCOMPARE(fixture.viewModel.errorMessage(),
             QStringLiteral("发生日期、时间或时区无效"));
    QVERIFY(fixture.transactions.findAll().empty());
}

void CreateTransactionPresentationTest::reportsSetupQueryFailure()
{
    Fixture fixture;
    fixture.accounts.setFindAllShouldSucceed(false);

    fixture.viewModel.loadSetup();

    QVERIFY(!fixture.viewModel.isReady());
    QVERIFY(fixture.viewModel.hasError());
    QCOMPARE(fixture.viewModel.errorMessage(),
             QStringLiteral("无法读取账户和分类"));
    QCOMPARE(fixture.viewModel.availableAccounts()->rowCount(), 0);
    QCOMPARE(fixture.viewModel.availableCategories()->rowCount(), 0);
}

QTEST_APPLESS_MAIN(CreateTransactionPresentationTest)

#include "CreateTransactionPresentationTest.moc"
