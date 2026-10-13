#include "DeleteScheduleItem.h"
#include "GetAcademicTerms.h"
#include "GetScheduleSetup.h"
#include "GetWeeklySchedule.h"
#include "SaveAcademicTerm.h"
#include "SaveCourse.h"
#include "SaveCourseMeeting.h"
#include "ScheduleManagementModels.h"
#include "ScheduleManagementViewModel.h"
#include "ScheduleViewModel.h"
#include "WeeklyScheduleListModel.h"
#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseMeetingRepository.h"
#include "support/InMemoryCourseRepository.h"
#include "support/ScheduleApplicationTestData.h"

#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>

using namespace stukit::schedule;
using namespace stukit::schedule::presentation;
using namespace stukit::schedule::testing;

namespace {

AcademicTerm makeTerm(
    const QUuid &id,
    const QString &name,
    const QDate &startDate,
    int totalWeeks,
    bool archived = false)
{
    return AcademicTerm::create(AcademicTermData{
        .id = id,
        .name = name,
        .startDate = startDate,
        .totalWeeks = totalWeeks,
        .archived = archived,
    }).value();
}

struct Fixture
{
    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    GetAcademicTerms getTerms{terms};
    GetScheduleSetup getSetup{terms, courses, meetings};
    GetWeeklySchedule getSchedule{terms, courses, meetings};
    SaveAcademicTerm saveTerm{terms, courses, meetings};
    SaveCourse saveCourse{terms, courses};
    SaveCourseMeeting saveMeeting{terms, courses, meetings};
    DeleteScheduleItem deleteItem{terms, courses, meetings};
};

} // namespace

class SchedulePresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void exposesReadableModelRoles();
    void loadsCurrentTermAndWeek();
    void navigatesWeeksAndSelectsTerm();
    void handlesEmptyTermList();
    void clearsStaleScheduleOnFailureAndRecovers();
    void clearsSelectionWhenTermQueryFails();
    void managesTermsCoursesAndMeetings();
    void reportsMeetingConflictsWithoutSaving();
    void clearsManagementModelsOnFailure();
};

void SchedulePresentationTest::exposesReadableModelRoles()
{
    const QUuid courseId = QUuid::createUuid();
    const QUuid meetingId = QUuid::createUuid();
    WeeklyScheduleListModel model;
    model.replaceItems({WeeklyScheduleItem{
        .courseId = courseId,
        .meetingId = meetingId,
        .courseName = QStringLiteral("高等数学"),
        .teacher = QStringLiteral("王老师"),
        .location = QStringLiteral("A101"),
        .color = QStringLiteral("#3B82F6"),
        .weekday = Weekday::Wednesday,
        .startPeriod = 3,
        .endPeriod = 4,
        .startWeek = 1,
        .endWeek = 18,
        .weekPattern = WeekPattern::EveryWeek,
    }});

    QCOMPARE(model.rowCount(), 1);
    const QModelIndex index = model.index(0);
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::CourseIdRole).toString(),
        courseId.toString(QUuid::WithoutBraces));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::MeetingIdRole).toString(),
        meetingId.toString(QUuid::WithoutBraces));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::CourseNameRole).toString(),
        QStringLiteral("高等数学"));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::TeacherRole).toString(),
        QStringLiteral("王老师"));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::LocationRole).toString(),
        QStringLiteral("A101"));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::ColorRole).toString(),
        QStringLiteral("#3B82F6"));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::WeekdayRole).toInt(), 3);
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::WeekdayTextRole).toString(),
        QStringLiteral("周三"));
    QCOMPARE(
        model.data(index, WeeklyScheduleListModel::PeriodTextRole).toString(),
        QStringLiteral("第 3～4 节"));
    QCOMPARE(
        model.data(
            index, WeeklyScheduleListModel::WeekRangeTextRole).toString(),
        QStringLiteral("第 1～18 周 · 每周"));
    QCOMPARE(
        model.roleNames().value(WeeklyScheduleListModel::CourseNameRole),
        QByteArrayLiteral("courseName"));
    QCOMPARE(model.weekdayCounts(), QVariantList({0, 0, 1, 0, 0, 0, 0}));

    model.clear();
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.weekdayCounts(), QVariantList({0, 0, 0, 0, 0, 0, 0}));
}

void SchedulePresentationTest::loadsCurrentTermAndWeek()
{
    Fixture fixture;
    const QUuid termId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    QVERIFY(fixture.terms.save(makeTerm(
        termId,
        QStringLiteral("2026 秋季学期"),
        QDate(2026, 9, 7),
        18)));
    QVERIFY(fixture.courses.save(applicationTestCourse(courseId, termId)));
    QVERIFY(fixture.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        courseId,
        Weekday::Monday,
        1,
        2,
        1,
        18,
        WeekPattern::EvenWeeks)));
    QVERIFY(fixture.meetings.save(applicationTestMeeting(
        QUuid::createUuid(),
        courseId,
        Weekday::Tuesday,
        3,
        4,
        1,
        18,
        WeekPattern::OddWeeks)));
    ScheduleViewModel viewModel(
        fixture.getTerms,
        fixture.getSchedule,
        QDate(2026, 9, 15));
    QSignalSpy scheduleSpy(
        &viewModel, &ScheduleViewModel::scheduleChanged);

    viewModel.load();

    QVERIFY(viewModel.hasTerms());
    QCOMPARE(viewModel.termOptions().size(), 1);
    QCOMPARE(viewModel.selectedTermId(),
             termId.toString(QUuid::WithoutBraces));
    QCOMPARE(viewModel.selectedTermName(), QStringLiteral("2026 秋季学期"));
    QCOMPARE(viewModel.selectedWeek(), 2);
    QCOMPARE(viewModel.totalWeeks(), 18);
    QCOMPARE(viewModel.weekLabel(), QStringLiteral("第 2 周"));
    QCOMPARE(
        viewModel.weekDateRange(),
        QStringLiteral("2026-09-14 ～ 2026-09-20"));
    const QVariantList weekDays = viewModel.weekDays();
    QCOMPARE(weekDays.size(), 7);
    QCOMPARE(
        weekDays.first().toMap().value(QStringLiteral("weekdayText")),
        QStringLiteral("周一"));
    QCOMPARE(
        weekDays.first().toMap().value(QStringLiteral("fullDateText")),
        QStringLiteral("2026-09-14"));
    QVERIFY(weekDays.at(1).toMap().value(QStringLiteral("isToday")).toBool());
    QVERIFY(viewModel.currentWeekAvailable());
    QVERIFY(viewModel.showingCurrentWeek());
    QVERIFY(viewModel.canShowPreviousWeek());
    QVERIFY(viewModel.canShowNextWeek());
    QCOMPARE(viewModel.meetings()->rowCount(), 1);
    QVERIFY(!viewModel.isEmpty());
    QVERIFY(!viewModel.hasError());
    QVERIFY(!viewModel.isLoading());
    QVERIFY(scheduleSpy.count() >= 1);
}

void SchedulePresentationTest::navigatesWeeksAndSelectsTerm()
{
    Fixture fixture;
    const QUuid activeTermId = QUuid::createUuid();
    const QUuid archivedTermId = QUuid::createUuid();
    QVERIFY(fixture.terms.save(makeTerm(
        activeTermId,
        QStringLiteral("短学期"),
        QDate(2026, 9, 7),
        2)));
    QVERIFY(fixture.terms.save(makeTerm(
        archivedTermId,
        QStringLiteral("历史学期"),
        QDate(2025, 9, 1),
        2,
        true)));
    ScheduleViewModel viewModel(
        fixture.getTerms,
        fixture.getSchedule,
        QDate(2026, 10, 11),
        activeTermId);

    viewModel.load();
    QCOMPARE(viewModel.selectedWeek(), 1);
    QVERIFY(!viewModel.currentWeekAvailable());
    QVERIFY(!viewModel.canShowPreviousWeek());

    viewModel.showNextWeek();
    QCOMPARE(viewModel.selectedWeek(), 2);
    QVERIFY(!viewModel.canShowNextWeek());
    viewModel.showNextWeek();
    QCOMPARE(viewModel.selectedWeek(), 2);
    viewModel.showPreviousWeek();
    QCOMPARE(viewModel.selectedWeek(), 1);

    QVERIFY(!viewModel.showCurrentWeek());
    QCOMPARE(
        viewModel.errorMessage(),
        QStringLiteral("当前日期不在所选学期内"));

    QVERIFY(viewModel.selectTerm(
        archivedTermId.toString(QUuid::WithoutBraces)));
    QCOMPARE(viewModel.selectedTermName(), QStringLiteral("历史学期"));
    QCOMPARE(viewModel.selectedWeek(), 1);
    const QVariantMap archivedOption = viewModel.termOptions()
        .at(1).toMap();
    QCOMPARE(
        archivedOption.value(QStringLiteral("label")).toString(),
        QStringLiteral("历史学期（已归档）"));

    QVERIFY(!viewModel.selectTerm(QStringLiteral("not-a-uuid")));
    QCOMPARE(viewModel.selectedTermName(), QStringLiteral("历史学期"));
}

void SchedulePresentationTest::handlesEmptyTermList()
{
    Fixture fixture;
    ScheduleViewModel viewModel(
        fixture.getTerms,
        fixture.getSchedule,
        QDate(2026, 10, 11));

    viewModel.load();

    QVERIFY(!viewModel.hasTerms());
    QVERIFY(viewModel.termOptions().isEmpty());
    QVERIFY(viewModel.selectedTermId().isEmpty());
    QCOMPARE(viewModel.selectedWeek(), 0);
    QCOMPARE(viewModel.weekLabel(), QStringLiteral("未选择教学周"));
    QVERIFY(viewModel.isEmpty());
    QVERIFY(!viewModel.hasError());
    QVERIFY(!viewModel.showCurrentWeek());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("请先选择学期"));
}

void SchedulePresentationTest::clearsStaleScheduleOnFailureAndRecovers()
{
    Fixture fixture;
    const QUuid termId = QUuid::createUuid();
    const QUuid courseId = QUuid::createUuid();
    QVERIFY(fixture.terms.save(makeTerm(
        termId,
        QStringLiteral("测试学期"),
        QDate(2026, 9, 7),
        18)));
    QVERIFY(fixture.courses.save(applicationTestCourse(courseId, termId)));
    QVERIFY(fixture.meetings.save(applicationTestMeeting(
        QUuid::createUuid(), courseId)));
    ScheduleViewModel viewModel(
        fixture.getTerms,
        fixture.getSchedule,
        QDate(2026, 9, 8));
    viewModel.load();
    QCOMPARE(viewModel.meetings()->rowCount(), 1);

    fixture.meetings.setQueryShouldSucceed(false);
    viewModel.showNextWeek();
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法读取课表数据"));
    QCOMPARE(viewModel.meetings()->rowCount(), 0);

    fixture.meetings.setQueryShouldSucceed(true);
    viewModel.showPreviousWeek();
    QVERIFY(!viewModel.hasError());
    QCOMPARE(viewModel.meetings()->rowCount(), 1);
}

void SchedulePresentationTest::clearsSelectionWhenTermQueryFails()
{
    Fixture fixture;
    const QUuid termId = QUuid::createUuid();
    QVERIFY(fixture.terms.save(makeTerm(
        termId,
        QStringLiteral("测试学期"),
        QDate(2026, 9, 7),
        18)));
    ScheduleViewModel viewModel(
        fixture.getTerms,
        fixture.getSchedule,
        QDate(2026, 9, 8));
    viewModel.load();
    QVERIFY(viewModel.hasTerms());

    fixture.terms.setQueryShouldSucceed(false);
    viewModel.load();

    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("无法读取学期数据"));
    QVERIFY(!viewModel.hasTerms());
    QVERIFY(viewModel.selectedTermId().isEmpty());
    QCOMPARE(viewModel.selectedWeek(), 0);
    QVERIFY(viewModel.isEmpty());
}

void SchedulePresentationTest::managesTermsCoursesAndMeetings()
{
    Fixture fixture;
    ScheduleManagementViewModel viewModel(
        fixture.getSetup,
        fixture.saveTerm,
        fixture.saveCourse,
        fixture.saveMeeting,
        fixture.deleteItem,
        nullptr,
        nullptr);
    QSignalSpy dataSpy(
        &viewModel, &ScheduleManagementViewModel::dataChanged);
    viewModel.load();
    QVERIFY(!viewModel.hasTerms());

    QVERIFY(viewModel.saveTerm(
        {},
        QStringLiteral("2026 秋季学期"),
        QStringLiteral("2026-09-07"),
        18,
        false));
    QVERIFY(viewModel.hasTerms());
    QCOMPARE(viewModel.terms()->rowCount(), 1);
    QVERIFY(!viewModel.selectedTermId().isEmpty());
    QCOMPARE(viewModel.selectedTermTotalWeeks(), 18);
    const QString termId = viewModel.selectedTermId();
    QVERIFY(!viewModel.saveTerm(
        QStringLiteral("invalid-id"),
        QStringLiteral("不应创建"),
        QStringLiteral("2026-09-07"),
        18,
        false));
    QCOMPARE(viewModel.terms()->rowCount(), 1);

    QVERIFY(viewModel.saveCourseDetails(
        {},
        termId,
        QStringLiteral("高等数学"),
        QStringLiteral("王老师"),
        QStringLiteral("A101"),
        QStringLiteral("#3b82f6"),
        QStringLiteral("平时 30%，期末 70%"),
        QStringLiteral("第 1～4 周：基础章节")));
    QVERIFY(viewModel.hasCourses());
    QCOMPARE(viewModel.courses()->rowCount(), 1);
    const QString courseId = viewModel.selectedCourseId();
    const QModelIndex courseIndex = viewModel.courses()->index(0, 0);
    QCOMPARE(
        viewModel.courses()->data(
            courseIndex, CourseListModel::ColorRole).toString(),
        QStringLiteral("#3B82F6"));
    QCOMPARE(
        viewModel.courses()->data(
            courseIndex,
            CourseListModel::AssessmentStandardRole).toString(),
        QStringLiteral("平时 30%，期末 70%"));
    QCOMPARE(
        viewModel.courses()->data(
            courseIndex, CourseListModel::TeachingPlanRole).toString(),
        QStringLiteral("第 1～4 周：基础章节"));

    QVERIFY(viewModel.saveMeetingDetails(
        {}, courseId, 1, 1, 2, 1, 18, 0));
    QVERIFY(viewModel.hasMeetings());
    QCOMPARE(viewModel.meetings()->rowCount(), 1);
    QCOMPARE(viewModel.termSchedule()->rowCount(), 1);
    const QModelIndex meetingIndex = viewModel.meetings()->index(0, 0);
    QCOMPARE(
        viewModel.meetings()->data(
            meetingIndex,
            CourseMeetingListModel::WeekdayTextRole).toString(),
        QStringLiteral("周一"));
    QVERIFY(viewModel.meetings()->data(
        meetingIndex,
        CourseMeetingListModel::SummaryRole).toString().contains(
            QStringLiteral("第 1～2 节")));
    const QModelIndex termScheduleIndex =
        viewModel.termSchedule()->index(0, 0);
    QCOMPARE(
        viewModel.termSchedule()->data(
            termScheduleIndex,
            WeeklyScheduleListModel::CourseNameRole).toString(),
        QStringLiteral("高等数学"));
    QCOMPARE(
        viewModel.termSchedule()->data(
            termScheduleIndex,
            WeeklyScheduleListModel::WeekRangeTextRole).toString(),
        QStringLiteral("第 1～18 周 · 每周"));

    QVERIFY(viewModel.setTermArchived(termId, true));
    QVERIFY(viewModel.selectedTermArchived());
    QVERIFY(!viewModel.saveCourseDetails(
        {},
        termId,
        QStringLiteral("线性代数"),
        {},
        {},
        QStringLiteral("#10B981")));
    QCOMPARE(
        viewModel.errorMessage(),
        QStringLiteral("已归档学期不能新增或修改课程"));
    QCOMPARE(dataSpy.count(), 4);
}

void SchedulePresentationTest::reportsMeetingConflictsWithoutSaving()
{
    Fixture fixture;
    ScheduleManagementViewModel viewModel(
        fixture.getSetup,
        fixture.saveTerm,
        fixture.saveCourse,
        fixture.saveMeeting,
        fixture.deleteItem,
        nullptr,
        nullptr);
    viewModel.load();
    QVERIFY(viewModel.saveTerm(
        {}, QStringLiteral("测试学期"), QStringLiteral("2026-09-07"),
        18, false));
    const QString termId = viewModel.selectedTermId();
    QVERIFY(viewModel.saveCourseDetails(
        {}, termId, QStringLiteral("高等数学"), {}, {},
        QStringLiteral("#3B82F6")));
    const QString firstCourseId = viewModel.selectedCourseId();
    QVERIFY(viewModel.saveMeetingDetails(
        {}, firstCourseId, 1, 1, 2, 1, 18, 0));

    QVERIFY(viewModel.saveCourseDetails(
        {}, termId, QStringLiteral("大学英语"), {}, {},
        QStringLiteral("#EF4444")));
    const QString secondCourseId = viewModel.selectedCourseId();
    QVERIFY(!viewModel.saveMeetingDetails(
        {}, secondCourseId, 1, 2, 3, 1, 18, 0));

    QVERIFY(viewModel.hasConflicts());
    QCOMPARE(viewModel.conflicts().size(), 1);
    QCOMPARE(
        viewModel.conflicts().front().toMap()
            .value(QStringLiteral("courseName")).toString(),
        QStringLiteral("高等数学"));
    QCOMPARE(viewModel.meetings()->rowCount(), 0);
    QCOMPARE(fixture.meetings.findAll()->size(), std::size_t{1});
}

void SchedulePresentationTest::clearsManagementModelsOnFailure()
{
    Fixture fixture;
    QVERIFY(fixture.terms.save(makeTerm(
        QUuid::createUuid(),
        QStringLiteral("测试学期"),
        QDate(2026, 9, 7),
        18)));
    ScheduleManagementViewModel viewModel(
        fixture.getSetup,
        fixture.saveTerm,
        fixture.saveCourse,
        fixture.saveMeeting,
        fixture.deleteItem,
        nullptr,
        nullptr);
    viewModel.load();
    QCOMPARE(viewModel.terms()->rowCount(), 1);

    fixture.courses.setQueryShouldSucceed(false);
    viewModel.load();

    QVERIFY(viewModel.hasError());
    QCOMPARE(
        viewModel.errorMessage(),
        QStringLiteral("无法读取课表管理数据"));
    QCOMPARE(viewModel.terms()->rowCount(), 0);
    QCOMPARE(viewModel.courses()->rowCount(), 0);
    QCOMPARE(viewModel.meetings()->rowCount(), 0);
}

QTEST_APPLESS_MAIN(SchedulePresentationTest)

#include "SchedulePresentationTest.moc"
