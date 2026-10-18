#include "AccountingSetupViewModel.h"
#include "GetAccountingSetup.h"
#include "MoneyInputParser.h"
#include "UpdateAccount.h"
#include "UpdateCategory.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"
#include "support/InMemoryTransactionRepository.h"

#include <QAbstractItemModel>
#include <QTest>
#include <QUuid>

#include <limits>
#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::presentation;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount(const QString &name, bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = name,
        .openingBalance = Money::fromCents(1234),
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

} // namespace

class AccountingSetupTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesExactMoneyText();
    void rejectsInvalidOrOverflowingMoneyText();
    void returnsEmptyOrExistingSetup();
    void reportsEitherRepositoryFailure();
    void loadsModelsAndCreatesSetupData();
    void exposesStableInputAndStorageErrors();
    void renamesArchivesAndRestoresSetupData();
    void deletesOnlyUnusedArchivedSetupData();
};

void AccountingSetupTest::parsesExactMoneyText()
{
    QCOMPARE(parseMoneyInput(QStringLiteral("0"))->cents(), qint64{0});
    QCOMPARE(parseMoneyInput(QStringLiteral("12"))->cents(), qint64{1200});
    QCOMPARE(parseMoneyInput(QStringLiteral("12.3"))->cents(), qint64{1230});
    QCOMPARE(parseMoneyInput(QStringLiteral("+12.34"))->cents(), qint64{1234});
    QCOMPARE(parseMoneyInput(QStringLiteral(" -12.34 "))->cents(), qint64{-1234});
    QCOMPARE(parseMoneyInput(QStringLiteral("92233720368547758.07"))->cents(),
             std::numeric_limits<qint64>::max());
    QCOMPARE(parseMoneyInput(QStringLiteral("-92233720368547758.08"))->cents(),
             std::numeric_limits<qint64>::min());
}

void AccountingSetupTest::rejectsInvalidOrOverflowingMoneyText()
{
    const QStringList invalidValues{
        QString{},
        QStringLiteral("+"),
        QStringLiteral(".5"),
        QStringLiteral("12."),
        QStringLiteral("12.345"),
        QStringLiteral("1.2.3"),
        QStringLiteral("1e3"),
        QStringLiteral("NaN"),
        QStringLiteral("92233720368547758.08"),
        QStringLiteral("-92233720368547758.09"),
    };

    for (const QString &value : invalidValues) {
        QVERIFY2(!parseMoneyInput(value).has_value(), qPrintable(value));
    }
}

void AccountingSetupTest::returnsEmptyOrExistingSetup()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    GetAccountingSetup getSetup(accounts, categories);

    auto result = getSetup.execute();
    QVERIFY(std::holds_alternative<AccountingSetup>(result));
    QVERIFY(std::get<AccountingSetup>(result).accounts.empty());
    QVERIFY(std::get<AccountingSetup>(result).categories.empty());

    QVERIFY(accounts.save(makeAccount(QStringLiteral("现金"))));
    QVERIFY(categories.save(makeCategory(
        QStringLiteral("工资"), TransactionType::Income)));
    result = getSetup.execute();
    QVERIFY(std::holds_alternative<AccountingSetup>(result));
    QCOMPARE(std::get<AccountingSetup>(result).accounts.size(), std::size_t{1});
    QCOMPARE(std::get<AccountingSetup>(result).categories.size(), std::size_t{1});
}

void AccountingSetupTest::reportsEitherRepositoryFailure()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    GetAccountingSetup getSetup(accounts, categories);

    accounts.setFindAllShouldSucceed(false);
    QVERIFY(std::holds_alternative<GetAccountingSetupError>(getSetup.execute()));

    accounts.setFindAllShouldSucceed(true);
    categories.setFindAllShouldSucceed(false);
    QVERIFY(std::holds_alternative<GetAccountingSetupError>(getSetup.execute()));
}

void AccountingSetupTest::loadsModelsAndCreatesSetupData()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    QVERIFY(accounts.save(makeAccount(QStringLiteral("已归档"), true)));
    QVERIFY(categories.save(makeCategory(
        QStringLiteral("旧分类"), TransactionType::Expense, true)));
    CreateAccount createAccount(accounts);
    CreateCategory createCategory(categories);
    UpdateAccount updateAccount(accounts);
    UpdateCategory updateCategory(categories);
    InMemoryTransactionRepository transactions;
    DeleteAccountingSetupItem deleteSetupItem(accounts, categories, transactions);
    GetAccountingSetup getSetup(accounts, categories);
    AccountingSetupViewModel viewModel(
        getSetup, createAccount, createCategory, updateAccount, updateCategory,
        deleteSetupItem);

    viewModel.load();
    QCOMPARE(viewModel.accounts()->rowCount(), 1);
    QCOMPARE(viewModel.categories()->rowCount(), 1);
    QVERIFY(!viewModel.hasAccounts());
    QVERIFY(!viewModel.hasIncomeCategories());
    QVERIFY(!viewModel.hasExpenseCategories());

    QVERIFY(viewModel.createAccount(QStringLiteral("现金"), QStringLiteral("12.34")));
    QVERIFY(viewModel.createCategory(QStringLiteral("工资"), true));
    QVERIFY(viewModel.createCategory(QStringLiteral("餐饮"), false));
    QCOMPARE(viewModel.accounts()->rowCount(), 2);
    QCOMPARE(viewModel.categories()->rowCount(), 3);
    QVERIFY(viewModel.hasAccounts());
    QVERIFY(viewModel.hasIncomeCategories());
    QVERIFY(viewModel.hasExpenseCategories());
    QVERIFY(!viewModel.hasError());

    const QModelIndex accountIndex = viewModel.accounts()->index(1, 0);
    QCOMPARE(viewModel.accounts()->data(accountIndex, AccountListModel::NameRole),
             QVariant(QStringLiteral("现金")));
    QCOMPARE(viewModel.accounts()->data(
                 accountIndex, AccountListModel::OpeningBalanceTextRole),
             QVariant(QStringLiteral("12.34")));
}

void AccountingSetupTest::exposesStableInputAndStorageErrors()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    CreateAccount createAccount(accounts);
    CreateCategory createCategory(categories);
    UpdateAccount updateAccount(accounts);
    UpdateCategory updateCategory(categories);
    InMemoryTransactionRepository transactions;
    DeleteAccountingSetupItem deleteSetupItem(accounts, categories, transactions);
    GetAccountingSetup getSetup(accounts, categories);
    AccountingSetupViewModel viewModel(
        getSetup, createAccount, createCategory, updateAccount, updateCategory,
        deleteSetupItem);

    QVERIFY(!viewModel.createAccount(QStringLiteral("现金"), QStringLiteral("1.234")));
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("期初余额格式无效"));
    QVERIFY(!viewModel.createAccount(QStringLiteral("   "), QStringLiteral("0")));
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("账户名称无效"));

    categories.setSaveShouldSucceed(false);
    QVERIFY(!viewModel.createCategory(QStringLiteral("餐饮"), false));
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法保存分类"));

    categories.setSaveShouldSucceed(true);
    accounts.setFindAllShouldSucceed(false);
    viewModel.load();
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法读取账户和分类"));
}

void AccountingSetupTest::renamesArchivesAndRestoresSetupData()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const Account account = makeAccount(QStringLiteral("现金"));
    const Category category = makeCategory(
        QStringLiteral("餐饮"), TransactionType::Expense);
    QVERIFY(accounts.save(account));
    QVERIFY(categories.save(category));
    CreateAccount createAccount(accounts);
    CreateCategory createCategory(categories);
    UpdateAccount updateAccount(accounts);
    UpdateCategory updateCategory(categories);
    InMemoryTransactionRepository transactions;
    DeleteAccountingSetupItem deleteSetupItem(accounts, categories, transactions);
    GetAccountingSetup getSetup(accounts, categories);
    AccountingSetupViewModel viewModel(
        getSetup, createAccount, createCategory, updateAccount, updateCategory,
        deleteSetupItem);
    viewModel.load();

    const QString accountId = account.id().toString(QUuid::WithoutBraces);
    const QString categoryId = category.id().toString(QUuid::WithoutBraces);
    QVERIFY(viewModel.renameAccount(accountId, QStringLiteral("主现金账户")));
    QVERIFY(viewModel.renameCategory(categoryId, QStringLiteral("日常餐饮")));
    QCOMPARE(viewModel.accounts()->data(
                 viewModel.accounts()->index(0, 0), AccountListModel::NameRole),
             QVariant(QStringLiteral("主现金账户")));
    QCOMPARE(viewModel.categories()->data(
                 viewModel.categories()->index(0, 0), CategoryListModel::NameRole),
             QVariant(QStringLiteral("日常餐饮")));

    QVERIFY(viewModel.setAccountArchived(accountId, true));
    QVERIFY(viewModel.setCategoryArchived(categoryId, true));
    QVERIFY(!viewModel.hasAccounts());
    QVERIFY(!viewModel.hasExpenseCategories());
    QVERIFY(viewModel.accounts()->data(
        viewModel.accounts()->index(0, 0), AccountListModel::ArchivedRole).toBool());
    QVERIFY(viewModel.categories()->data(
        viewModel.categories()->index(0, 0), CategoryListModel::ArchivedRole).toBool());

    QVERIFY(viewModel.setAccountArchived(accountId, false));
    QVERIFY(viewModel.setCategoryArchived(categoryId, false));
    QVERIFY(viewModel.hasAccounts());
    QVERIFY(viewModel.hasExpenseCategories());

    QVERIFY(!viewModel.renameAccount(QStringLiteral("not-a-uuid"),
                                     QStringLiteral("不存在")));
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("账户信息无效"));
}

void AccountingSetupTest::deletesOnlyUnusedArchivedSetupData()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    InMemoryTransactionRepository transactions;
    DeleteAccountingSetupItem deleteSetupItem(accounts, categories, transactions);

    const Account activeAccount = makeAccount(QStringLiteral("活动账户"));
    const Account archivedAccount = makeAccount(QStringLiteral("归档账户"), true);
    const Category archivedCategory = makeCategory(
        QStringLiteral("归档分类"), TransactionType::Expense, true);
    QVERIFY(accounts.save(activeAccount));
    QVERIFY(accounts.save(archivedAccount));
    QVERIFY(categories.save(archivedCategory));

    QCOMPARE(
        deleteSetupItem.deleteAccount(activeAccount.id()),
        DeleteAccountingSetupItemError::ItemNotArchived);
    QCOMPARE(
        deleteSetupItem.deleteAccount(archivedAccount.id()),
        DeleteAccountingSetupItem::Success);
    QVERIFY(!accounts.findById(archivedAccount.id()).has_value());

    const Account referencedAccount = makeAccount(
        QStringLiteral("历史账户"), true);
    QVERIFY(accounts.save(referencedAccount));
    const auto transaction = Transaction::create(TransactionData{
        .id = QUuid::createUuid(),
        .type = TransactionType::Expense,
        .amount = Money::fromCents(1200),
        .accountId = referencedAccount.id(),
        .categoryId = archivedCategory.id(),
        .occurredAt = QDateTime::currentDateTimeUtc(),
        .note = QStringLiteral("历史账单"),
    });
    QVERIFY(transaction.has_value());
    QVERIFY(transactions.save(*transaction));

    QCOMPARE(
        deleteSetupItem.deleteAccount(referencedAccount.id()),
        DeleteAccountingSetupItemError::ItemInUse);
    QCOMPARE(
        deleteSetupItem.deleteCategory(archivedCategory.id()),
        DeleteAccountingSetupItemError::ItemInUse);
    QVERIFY(accounts.findById(referencedAccount.id()).has_value());
    QVERIFY(categories.findById(archivedCategory.id()).has_value());
}

QTEST_APPLESS_MAIN(AccountingSetupTest)

#include "AccountingSetupTest.moc"
