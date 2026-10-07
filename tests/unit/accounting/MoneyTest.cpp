#include <QtTest>

#include "Money.h"

using stukit::accounting::Money;

class MoneyTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsFromCents();
    void identifiesSign();
    void addsValues();
    void subtractsValues();
    void comparesByValue();
};

void MoneyTest::createsFromCents()
{
    const Money money = Money::fromCents(12345);

    QCOMPARE(money.cents(), qint64{12345});
}

void MoneyTest::identifiesSign()
{
    const Money positive = Money::fromCents(1);
    const Money zero = Money::fromCents(0);
    const Money negative = Money::fromCents(-1);

    QVERIFY(positive.isPositive());
    QVERIFY(!positive.isZero());
    QVERIFY(!positive.isNegative());

    QVERIFY(zero.isZero());
    QVERIFY(!zero.isPositive());
    QVERIFY(!zero.isNegative());

    QVERIFY(negative.isNegative());
    QVERIFY(!negative.isZero());
    QVERIFY(!negative.isPositive());
}

void MoneyTest::addsValues()
{
    const Money result = Money::fromCents(120) + Money::fromCents(30);

    QCOMPARE(result.cents(), qint64{150});
}

void MoneyTest::subtractsValues()
{
    const Money result = Money::fromCents(120) - Money::fromCents(150);

    QCOMPARE(result.cents(), qint64{-30});
}

void MoneyTest::comparesByValue()
{
    QVERIFY(Money::fromCents(500) == Money::fromCents(500));
    QVERIFY(Money::fromCents(500) != Money::fromCents(501));
}

QTEST_APPLESS_MAIN(MoneyTest)

#include "MoneyTest.moc"
