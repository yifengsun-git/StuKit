#include "CourseMeeting.h"

#include <QTest>

using namespace stukit::schedule;

namespace {

CourseMeetingData validMeetingData()
{
    return CourseMeetingData{
        .id = QUuid::createUuid(),
        .courseId = QUuid::createUuid(),
        .weekday = Weekday::Tuesday,
        .startPeriod = 3,
        .endPeriod = 4,
        .startWeek = 1,
        .endWeek = 18,
        .weekPattern = WeekPattern::EveryWeek,
    };
}

} // namespace

class CourseMeetingTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidMeeting();
    void calculatesOccurrence_data();
    void calculatesOccurrence();
    void rejectsNullId_data();
    void rejectsNullId();
    void rejectsInvalidWeekday_data();
    void rejectsInvalidWeekday();
    void rejectsInvalidPeriodRange_data();
    void rejectsInvalidPeriodRange();
    void rejectsInvalidWeekRange_data();
    void rejectsInvalidWeekRange();
    void rejectsInvalidWeekPattern();
    void rejectsRangeWithoutActiveWeek_data();
    void rejectsRangeWithoutActiveWeek();
};

void CourseMeetingTest::createsValidMeeting()
{
    const auto data = validMeetingData();

    const auto meeting = CourseMeeting::create(data);

    QVERIFY(meeting.has_value());
    QCOMPARE(meeting->id(), data.id);
    QCOMPARE(meeting->courseId(), data.courseId);
    QCOMPARE(meeting->weekday(), Weekday::Tuesday);
    QCOMPARE(meeting->startPeriod(), 3);
    QCOMPARE(meeting->endPeriod(), 4);
    QCOMPARE(meeting->startWeek(), 1);
    QCOMPARE(meeting->endWeek(), 18);
    QCOMPARE(meeting->weekPattern(), WeekPattern::EveryWeek);
}

void CourseMeetingTest::calculatesOccurrence_data()
{
    QTest::addColumn<WeekPattern>("pattern");
    QTest::addColumn<int>("week");
    QTest::addColumn<bool>("expected");

    QTest::newRow("every before range") << WeekPattern::EveryWeek << 0 << false;
    QTest::newRow("every first week") << WeekPattern::EveryWeek << 1 << true;
    QTest::newRow("every last week") << WeekPattern::EveryWeek << 18 << true;
    QTest::newRow("every after range") << WeekPattern::EveryWeek << 19 << false;
    QTest::newRow("odd matching") << WeekPattern::OddWeeks << 5 << true;
    QTest::newRow("odd not matching") << WeekPattern::OddWeeks << 6 << false;
    QTest::newRow("even matching") << WeekPattern::EvenWeeks << 6 << true;
    QTest::newRow("even not matching") << WeekPattern::EvenWeeks << 5 << false;
}

void CourseMeetingTest::calculatesOccurrence()
{
    QFETCH(WeekPattern, pattern);
    QFETCH(int, week);
    QFETCH(bool, expected);

    auto data = validMeetingData();
    data.weekPattern = pattern;
    const auto meeting = CourseMeeting::create(data);

    QVERIFY(meeting.has_value());
    QCOMPARE(meeting->occursInWeek(week), expected);
}

void CourseMeetingTest::rejectsNullId_data()
{
    QTest::addColumn<bool>("nullMeetingId");

    QTest::newRow("meeting id") << true;
    QTest::newRow("course id") << false;
}

void CourseMeetingTest::rejectsNullId()
{
    QFETCH(bool, nullMeetingId);

    auto data = validMeetingData();
    if (nullMeetingId) {
        data.id = QUuid{};
    } else {
        data.courseId = QUuid{};
    }

    QVERIFY(!CourseMeeting::create(data).has_value());
}

void CourseMeetingTest::rejectsInvalidWeekday_data()
{
    QTest::addColumn<Weekday>("weekday");

    QTest::newRow("below monday") << static_cast<Weekday>(0);
    QTest::newRow("above sunday") << static_cast<Weekday>(8);
}

void CourseMeetingTest::rejectsInvalidWeekday()
{
    QFETCH(Weekday, weekday);

    auto data = validMeetingData();
    data.weekday = weekday;

    QVERIFY(!CourseMeeting::create(data).has_value());
}

void CourseMeetingTest::rejectsInvalidPeriodRange_data()
{
    QTest::addColumn<int>("startPeriod");
    QTest::addColumn<int>("endPeriod");

    QTest::newRow("start below minimum") << 0 << 2;
    QTest::newRow("end above maximum") << 19 << 21;
    QTest::newRow("reversed") << 5 << 4;
}

void CourseMeetingTest::rejectsInvalidPeriodRange()
{
    QFETCH(int, startPeriod);
    QFETCH(int, endPeriod);

    auto data = validMeetingData();
    data.startPeriod = startPeriod;
    data.endPeriod = endPeriod;

    QVERIFY(!CourseMeeting::create(data).has_value());
}

void CourseMeetingTest::rejectsInvalidWeekRange_data()
{
    QTest::addColumn<int>("startWeek");
    QTest::addColumn<int>("endWeek");

    QTest::newRow("start below minimum") << 0 << 2;
    QTest::newRow("end above maximum") << 29 << 31;
    QTest::newRow("reversed") << 12 << 10;
}

void CourseMeetingTest::rejectsInvalidWeekRange()
{
    QFETCH(int, startWeek);
    QFETCH(int, endWeek);

    auto data = validMeetingData();
    data.startWeek = startWeek;
    data.endWeek = endWeek;

    QVERIFY(!CourseMeeting::create(data).has_value());
}

void CourseMeetingTest::rejectsInvalidWeekPattern()
{
    auto data = validMeetingData();
    data.weekPattern = static_cast<WeekPattern>(99);

    QVERIFY(!CourseMeeting::create(data).has_value());
}

void CourseMeetingTest::rejectsRangeWithoutActiveWeek_data()
{
    QTest::addColumn<int>("startWeek");
    QTest::addColumn<int>("endWeek");
    QTest::addColumn<WeekPattern>("pattern");

    QTest::newRow("odd pattern on even-only range")
        << 2 << 2 << WeekPattern::OddWeeks;
    QTest::newRow("even pattern on odd-only range")
        << 1 << 1 << WeekPattern::EvenWeeks;
}

void CourseMeetingTest::rejectsRangeWithoutActiveWeek()
{
    QFETCH(int, startWeek);
    QFETCH(int, endWeek);
    QFETCH(WeekPattern, pattern);

    auto data = validMeetingData();
    data.startWeek = startWeek;
    data.endWeek = endWeek;
    data.weekPattern = pattern;

    QVERIFY(!CourseMeeting::create(data).has_value());
}

QTEST_APPLESS_MAIN(CourseMeetingTest)

#include "CourseMeetingTest.moc"
