#include "AcademicTerm.h"

#include <QTest>

using namespace stukit::schedule;

namespace {

AcademicTermData validTermData()
{
    return AcademicTermData{
        .id = QUuid::createUuid(),
        .name = QStringLiteral("2026 秋季学期"),
        .startDate = QDate(2026, 9, 7),
        .totalWeeks = 18,
        .archived = false,
    };
}

} // namespace

class AcademicTermTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidTerm();
    void rejectsNullId();
    void rejectsInvalidName_data();
    void rejectsInvalidName();
    void rejectsInvalidDate();
    void rejectsInvalidTotalWeeks_data();
    void rejectsInvalidTotalWeeks();
    void preservesArchivedState();
};

void AcademicTermTest::createsValidTerm()
{
    auto data = validTermData();
    data.name = QStringLiteral("  2026 秋季学期  ");

    const auto term = AcademicTerm::create(data);

    QVERIFY(term.has_value());
    QCOMPARE(term->id(), data.id);
    QCOMPARE(term->name(), QStringLiteral("2026 秋季学期"));
    QCOMPARE(term->startDate(), QDate(2026, 9, 7));
    QCOMPARE(term->totalWeeks(), 18);
    QVERIFY(!term->isArchived());
}

void AcademicTermTest::rejectsNullId()
{
    auto data = validTermData();
    data.id = QUuid{};

    QVERIFY(!AcademicTerm::create(data).has_value());
}

void AcademicTermTest::rejectsInvalidName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("empty") << QString{};
    QTest::newRow("only whitespace") << QStringLiteral("   ");
    QTest::newRow("too long")
        << QString(AcademicTerm::MaximumNameLength + 1, QLatin1Char('a'));
}

void AcademicTermTest::rejectsInvalidName()
{
    QFETCH(QString, name);

    auto data = validTermData();
    data.name = name;

    QVERIFY(!AcademicTerm::create(data).has_value());
}

void AcademicTermTest::rejectsInvalidDate()
{
    auto data = validTermData();
    data.startDate = QDate{};

    QVERIFY(!AcademicTerm::create(data).has_value());
}

void AcademicTermTest::rejectsInvalidTotalWeeks_data()
{
    QTest::addColumn<int>("totalWeeks");

    QTest::newRow("zero") << 0;
    QTest::newRow("above maximum") << 31;
}

void AcademicTermTest::rejectsInvalidTotalWeeks()
{
    QFETCH(int, totalWeeks);

    auto data = validTermData();
    data.totalWeeks = totalWeeks;

    QVERIFY(!AcademicTerm::create(data).has_value());
}

void AcademicTermTest::preservesArchivedState()
{
    auto data = validTermData();
    data.archived = true;

    const auto term = AcademicTerm::create(data);

    QVERIFY(term.has_value());
    QVERIFY(term->isArchived());
}

QTEST_APPLESS_MAIN(AcademicTermTest)

#include "AcademicTermTest.moc"
