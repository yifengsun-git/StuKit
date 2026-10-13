#include "Course.h"

#include <QTest>

using namespace stukit::schedule;

namespace {

CourseData validCourseData()
{
    return CourseData{
        .id = QUuid::createUuid(),
        .termId = QUuid::createUuid(),
        .name = QStringLiteral("高等数学"),
        .teacher = QStringLiteral("王老师"),
        .location = QStringLiteral("教学楼 A101"),
        .assessmentStandard = QStringLiteral("平时 30%，期末 70%"),
        .teachingPlan = QStringLiteral("第 1～4 周：基础章节"),
        .color = QStringLiteral("#3B82F6"),
    };
}

} // namespace

class CourseTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsValidCourse();
    void allowsOptionalTeacherAndLocation();
    void rejectsNullCourseId();
    void rejectsNullTermId();
    void rejectsInvalidName_data();
    void rejectsInvalidName();
    void rejectsTooLongTeacher();
    void rejectsTooLongLocation();
    void rejectsTooLongCourseMaterials();
    void rejectsInvalidColor_data();
    void rejectsInvalidColor();
};

void CourseTest::createsValidCourse()
{
    auto data = validCourseData();
    data.name = QStringLiteral("  高等数学  ");
    data.teacher = QStringLiteral("  王老师  ");
    data.location = QStringLiteral("  教学楼 A101  ");
    data.assessmentStandard = QStringLiteral("  平时 30%，期末 70%  ");
    data.teachingPlan = QStringLiteral("  第 1～4 周：基础章节  ");
    data.color = QStringLiteral("  #3b82f6  ");

    const auto course = Course::create(data);

    QVERIFY(course.has_value());
    QCOMPARE(course->id(), data.id);
    QCOMPARE(course->termId(), data.termId);
    QCOMPARE(course->name(), QStringLiteral("高等数学"));
    QCOMPARE(course->teacher(), QStringLiteral("王老师"));
    QCOMPARE(course->location(), QStringLiteral("教学楼 A101"));
    QCOMPARE(
        course->assessmentStandard(), QStringLiteral("平时 30%，期末 70%"));
    QCOMPARE(
        course->teachingPlan(), QStringLiteral("第 1～4 周：基础章节"));
    QCOMPARE(course->color(), QStringLiteral("#3B82F6"));
}

void CourseTest::allowsOptionalTeacherAndLocation()
{
    auto data = validCourseData();
    data.teacher.clear();
    data.location = QStringLiteral("   ");

    const auto course = Course::create(data);

    QVERIFY(course.has_value());
    QVERIFY(course->teacher().isEmpty());
    QVERIFY(course->location().isEmpty());
}

void CourseTest::rejectsNullCourseId()
{
    auto data = validCourseData();
    data.id = QUuid{};

    QVERIFY(!Course::create(data).has_value());
}

void CourseTest::rejectsNullTermId()
{
    auto data = validCourseData();
    data.termId = QUuid{};

    QVERIFY(!Course::create(data).has_value());
}

void CourseTest::rejectsInvalidName_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("empty") << QString{};
    QTest::newRow("only whitespace") << QStringLiteral("   ");
    QTest::newRow("too long")
        << QString(Course::MaximumNameLength + 1, QLatin1Char('a'));
}

void CourseTest::rejectsInvalidName()
{
    QFETCH(QString, name);

    auto data = validCourseData();
    data.name = name;

    QVERIFY(!Course::create(data).has_value());
}

void CourseTest::rejectsTooLongTeacher()
{
    auto data = validCourseData();
    data.teacher = QString(Course::MaximumTeacherLength + 1, QLatin1Char('a'));

    QVERIFY(!Course::create(data).has_value());
}

void CourseTest::rejectsTooLongLocation()
{
    auto data = validCourseData();
    data.location = QString(Course::MaximumLocationLength + 1, QLatin1Char('a'));

    QVERIFY(!Course::create(data).has_value());
}

void CourseTest::rejectsInvalidColor_data()
{
    QTest::addColumn<QString>("color");

    QTest::newRow("empty") << QString{};
    QTest::newRow("missing hash") << QStringLiteral("3B82F6");
    QTest::newRow("short form") << QStringLiteral("#38F");
    QTest::newRow("too long") << QStringLiteral("#3B82F6AA");
    QTest::newRow("non hex") << QStringLiteral("#3B82FG");
}

void CourseTest::rejectsTooLongCourseMaterials()
{
    auto assessmentData = validCourseData();
    assessmentData.assessmentStandard = QString(
        Course::MaximumAssessmentStandardLength + 1, QLatin1Char('a'));
    QVERIFY(!Course::create(assessmentData).has_value());

    auto teachingPlanData = validCourseData();
    teachingPlanData.teachingPlan = QString(
        Course::MaximumTeachingPlanLength + 1, QLatin1Char('a'));
    QVERIFY(!Course::create(teachingPlanData).has_value());
}

void CourseTest::rejectsInvalidColor()
{
    QFETCH(QString, color);

    auto data = validCourseData();
    data.color = color;

    QVERIFY(!Course::create(data).has_value());
}

QTEST_APPLESS_MAIN(CourseTest)

#include "CourseTest.moc"
