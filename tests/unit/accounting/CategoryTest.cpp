#include "Category.h"

#include <QTest>

using namespace stukit::accounting;

namespace {

CategoryData validCategoryData()
{
    return CategoryData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("餐饮"),
        .type = TransactionType::Expense,
        .archived = false,
    };
}

} // namespace

class CategoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsExpenseCategory();
    void createsIncomeCategory();
    void rejectsNullId();
    void rejectsInvalidName_data();
    void rejectsInvalidName();
    void acceptsMaximumLengthName();
    void preservesArchivedState();
};

void CategoryTest::createsExpenseCategory()
{
    auto data = validCategoryData();
    data.name = QStringLiteral("  餐饮  ");

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QCOMPARE(category->id(), data.id);
    QCOMPARE(category->name(), QStringLiteral("餐饮"));
    QVERIFY(category->type() == TransactionType::Expense);
    QVERIFY(!category->isArchived());
}

void CategoryTest::createsIncomeCategory()
{
    auto data = validCategoryData();
    data.name = QStringLiteral("工资");
    data.type = TransactionType::Income;

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QVERIFY(category->type() == TransactionType::Income);
}

void CategoryTest::rejectsNullId()
{
    auto data = validCategoryData();
    data.id = QUuid{};

    QVERIFY(!Category::create(data).has_value());
}

void CategoryTest::rejectsInvalidName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("empty") << QString{};
    QTest::newRow("only whitespace") << QStringLiteral("   ");
    QTest::newRow("too long")
        << QString(Category::MaximumNameLength + 1, QLatin1Char('a'));
}

void CategoryTest::rejectsInvalidName()
{
    QFETCH(QString, name);

    auto data = validCategoryData();
    data.name = name;

    QVERIFY(!Category::create(data).has_value());
}

void CategoryTest::acceptsMaximumLengthName()
{
    auto data = validCategoryData();
    data.name = QString(Category::MaximumNameLength, QLatin1Char('a'));

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QCOMPARE(category->name().size(), Category::MaximumNameLength);
}

void CategoryTest::preservesArchivedState()
{
    auto data = validCategoryData();
    data.archived = true;

    const auto category = Category::create(data);

    QVERIFY(category.has_value());
    QVERIFY(category->isArchived());
}

QTEST_APPLESS_MAIN(CategoryTest)

#include "CategoryTest.moc"
