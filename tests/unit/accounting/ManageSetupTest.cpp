#include "UpdateAccount.h"
#include "UpdateCategory.h"
#include "support/InMemoryAccountRepository.h"
#include "support/InMemoryCategoryRepository.h"

#include <QTest>
#include <QUuid>

#include <variant>

using namespace stukit::accounting;
using namespace stukit::accounting::testing;

namespace {

Account makeAccount()
{
    return *Account::create(AccountData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("现金"),
        .openingBalance = Money::fromCents(12345),
        .archived = false,
    });
}

Category makeCategory()
{
    return *Category::create(CategoryData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("餐饮"),
        .type = TransactionType::Expense,
        .archived = false,
    });
}

} // namespace

class ManageSetupTest final : public QObject
{
    Q_OBJECT

private slots:
    void updatesAccountWithoutChangingIdentityOrBalance();
    void updatesCategoryWithoutChangingIdentityOrType();
    void rejectsInvalidOrMissingEntities();
    void reportsStorageFailures();
};

void ManageSetupTest::updatesAccountWithoutChangingIdentityOrBalance()
{
    InMemoryAccountRepository repository;
    const Account original = makeAccount();
    QVERIFY(repository.save(original));
    UpdateAccount update(repository);

    auto result = update.execute(UpdateAccountRequest{
        .id = original.id(),
        .name = QStringLiteral("主银行卡"),
        .archived = std::nullopt,
    });
    QVERIFY(std::holds_alternative<Account>(result));
    const Account renamed = std::get<Account>(result);
    QCOMPARE(renamed.id(), original.id());
    QCOMPARE(renamed.name(), QStringLiteral("主银行卡"));
    QCOMPARE(renamed.openingBalance().cents(), qint64{12345});
    QVERIFY(!renamed.isArchived());

    result = update.execute(UpdateAccountRequest{
        .id = original.id(),
        .name = std::nullopt,
        .archived = true,
    });
    QVERIFY(std::holds_alternative<Account>(result));
    const Account archived = std::get<Account>(result);
    QCOMPARE(archived.name(), QStringLiteral("主银行卡"));
    QCOMPARE(archived.openingBalance().cents(), qint64{12345});
    QVERIFY(archived.isArchived());
}

void ManageSetupTest::updatesCategoryWithoutChangingIdentityOrType()
{
    InMemoryCategoryRepository repository;
    const Category original = makeCategory();
    QVERIFY(repository.save(original));
    UpdateCategory update(repository);

    auto result = update.execute(UpdateCategoryRequest{
        .id = original.id(),
        .name = QStringLiteral("日常餐饮"),
        .archived = true,
    });
    QVERIFY(std::holds_alternative<Category>(result));
    const Category updated = std::get<Category>(result);
    QCOMPARE(updated.id(), original.id());
    QCOMPARE(updated.name(), QStringLiteral("日常餐饮"));
    QCOMPARE(updated.type(), TransactionType::Expense);
    QVERIFY(updated.isArchived());

    result = update.execute(UpdateCategoryRequest{
        .id = original.id(),
        .name = std::nullopt,
        .archived = false,
    });
    QVERIFY(std::holds_alternative<Category>(result));
    QVERIFY(!std::get<Category>(result).isArchived());
}

void ManageSetupTest::rejectsInvalidOrMissingEntities()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    UpdateAccount updateAccount(accounts);
    UpdateCategory updateCategory(categories);

    auto accountResult = updateAccount.execute(UpdateAccountRequest{
        .id = {},
        .name = QStringLiteral("现金"),
        .archived = std::nullopt,
    });
    QCOMPARE(std::get<UpdateAccountError>(accountResult),
             UpdateAccountError::InvalidInput);

    accountResult = updateAccount.execute(UpdateAccountRequest{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("现金"),
        .archived = std::nullopt,
    });
    QCOMPARE(std::get<UpdateAccountError>(accountResult),
             UpdateAccountError::NotFound);

    auto categoryResult = updateCategory.execute(UpdateCategoryRequest{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("餐饮"),
        .archived = std::nullopt,
    });
    QCOMPARE(std::get<UpdateCategoryError>(categoryResult),
             UpdateCategoryError::NotFound);
}

void ManageSetupTest::reportsStorageFailures()
{
    InMemoryAccountRepository accounts;
    InMemoryCategoryRepository categories;
    const Account account = makeAccount();
    const Category category = makeCategory();
    QVERIFY(accounts.save(account));
    QVERIFY(categories.save(category));
    accounts.setSaveShouldSucceed(false);
    categories.setSaveShouldSucceed(false);
    UpdateAccount updateAccount(accounts);
    UpdateCategory updateCategory(categories);

    const auto accountResult = updateAccount.execute(UpdateAccountRequest{
        .id = account.id(),
        .name = QStringLiteral("银行卡"),
        .archived = std::nullopt,
    });
    QCOMPARE(std::get<UpdateAccountError>(accountResult),
             UpdateAccountError::StorageFailure);

    const auto categoryResult = updateCategory.execute(UpdateCategoryRequest{
        .id = category.id(),
        .name = QStringLiteral("外卖"),
        .archived = std::nullopt,
    });
    QCOMPARE(std::get<UpdateCategoryError>(categoryResult),
             UpdateCategoryError::StorageFailure);
}

QTEST_APPLESS_MAIN(ManageSetupTest)

#include "ManageSetupTest.moc"
