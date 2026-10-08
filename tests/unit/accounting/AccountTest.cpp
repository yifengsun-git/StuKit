#include "Account.h"

#include <QTest>

using namespace stukit::accounting;

namespace {

AccountData validAccountData()
{
    return AccountData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("现金"),
        .openingBalance = Money::fromCents(10000),
        .archived = false,
    };
}

} // namespace

class AccountTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidAccount();
    void rejectsNullId();
    void rejectsInvalidName_data();
    void rejectsInvalidName();
    void allowsNegativeOpeningBalance();
    void preservesArchivedState();
};

void AccountTest::createsValidAccount()
{
    auto data = validAccountData();
    data.name = QStringLiteral("  现金  ");

    const auto account = Account::create(data);

    QVERIFY(account.has_value());
    QCOMPARE(account->id(), data.id);
    QCOMPARE(account->name(), QStringLiteral("现金"));
    QVERIFY(account->openingBalance() == Money::fromCents(10000));
    QVERIFY(!account->isArchived());
}

void AccountTest::rejectsNullId()
{
    auto data = validAccountData();
    data.id = QUuid{};

    QVERIFY(!Account::create(data).has_value());
}

void AccountTest::rejectsInvalidName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("empty") << QString{};
    QTest::newRow("only whitespace") << QStringLiteral("   ");
    QTest::newRow("too long")
        << QString(Account::MaximumNameLength + 1, QLatin1Char('a'));
}

void AccountTest::rejectsInvalidName()
{
    QFETCH(QString, name);

    auto data = validAccountData();
    data.name = name;

    QVERIFY(!Account::create(data).has_value());
}

void AccountTest::allowsNegativeOpeningBalance()
{
    auto data = validAccountData();
    data.openingBalance = Money::fromCents(-5000);

    const auto account = Account::create(data);

    QVERIFY(account.has_value());
    QVERIFY(account->openingBalance() == Money::fromCents(-5000));
}

void AccountTest::preservesArchivedState()
{
    auto data = validAccountData();
    data.archived = true;

    const auto account = Account::create(data);

    QVERIFY(account.has_value());
    QVERIFY(account->isArchived());
}

QTEST_APPLESS_MAIN(AccountTest)

#include "AccountTest.moc"
