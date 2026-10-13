#include "DeleteScheduleItem.h"
#include "GetAcademicTerms.h"
#include "GetScheduleSetup.h"
#include "GetWeeklySchedule.h"
#include "SaveAcademicTerm.h"
#include "SaveCourse.h"
#include "SaveCourseMeeting.h"
#include "ScheduleManagementViewModel.h"
#include "ScheduleViewModel.h"
#include "support/InMemoryAcademicTermRepository.h"
#include "support/InMemoryCourseMeetingRepository.h"
#include "support/InMemoryCourseRepository.h"
#include "support/ScheduleApplicationTestData.h"

#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QVariantMap>

#include <memory>

using namespace stukit::schedule;
using namespace stukit::schedule::presentation;
using namespace stukit::schedule::testing;

namespace {

struct Fixture
{
    explicit Fixture(bool seedMeetings, bool seedCourse = true)
        : termId(QUuid::createUuid())
        , courseId(QUuid::createUuid())
        , getTerms(terms)
        , getSetup(terms, courses, meetings)
        , getSchedule(terms, courses, meetings)
        , saveTerm(terms, courses, meetings)
        , saveCourse(terms, courses)
        , saveMeeting(terms, courses, meetings)
        , deleteItem(terms, courses, meetings)
        , viewModel(
              getTerms,
              getSchedule,
              QDate(2026, 9, 15),
              termId)
        , managementViewModel(
              getSetup,
              saveTerm,
              saveCourse,
              saveMeeting,
              deleteItem,
              nullptr,
              nullptr)
    {
        const bool setupSaved = terms.save(applicationTestTerm(termId))
            && (!seedCourse
                || courses.save(applicationTestCourse(courseId, termId)));
        Q_ASSERT(setupSaved);
        Q_UNUSED(setupSaved);

        if (seedMeetings && seedCourse) {
            const bool meetingsSaved = meetings.save(applicationTestMeeting(
                                           QUuid::createUuid(),
                                           courseId,
                                           Weekday::Monday,
                                           1,
                                           2))
                && meetings.save(applicationTestMeeting(
                    QUuid::createUuid(),
                    courseId,
                    Weekday::Monday,
                    4,
                    6))
                && meetings.save(applicationTestMeeting(
                    QUuid::createUuid(),
                    courseId,
                    Weekday::Wednesday,
                    8,
                    9,
                    1,
                    18,
                    WeekPattern::OddWeeks));
            Q_ASSERT(meetingsSaved);
            Q_UNUSED(meetingsSaved);
        }

        viewModel.load();
        managementViewModel.load();
        QObject::connect(
            &managementViewModel,
            &ScheduleManagementViewModel::dataChanged,
            &viewModel,
            &ScheduleViewModel::load);
    }

    InMemoryAcademicTermRepository terms;
    InMemoryCourseRepository courses;
    InMemoryCourseMeetingRepository meetings;
    QUuid termId;
    QUuid courseId;
    GetAcademicTerms getTerms;
    GetScheduleSetup getSetup;
    GetWeeklySchedule getSchedule;
    SaveAcademicTerm saveTerm;
    SaveCourse saveCourse;
    SaveCourseMeeting saveMeeting;
    DeleteScheduleItem deleteItem;
    ScheduleViewModel viewModel;
    ScheduleManagementViewModel managementViewModel;
};

std::unique_ptr<QObject> createSchedulePage(
    QQmlEngine &engine,
    ScheduleViewModel &viewModel,
    ScheduleManagementViewModel &managementViewModel,
    qreal width,
    qreal height = 720)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitScheduleTest"),
        QStringLiteral("ScheduleHomePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }

    QVariantMap properties{
        {QStringLiteral("viewModel"),
         QVariant::fromValue(static_cast<QObject *>(&viewModel))},
        {QStringLiteral("managementViewModel"),
         QVariant::fromValue(
             static_cast<QObject *>(&managementViewModel))},
        {QStringLiteral("startupError"), QString{}},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
    };
    std::unique_ptr<QObject> page(
        component.createWithInitialProperties(properties));
    if (!page) {
        qWarning().noquote() << component.errorString();
        return {};
    }
    QCoreApplication::processEvents();
    return page;
}

} // namespace

class ScheduleQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void switchesResponsiveLayouts();
    void rendersMeetingsAndNavigatesWeeks();
    void switchesToSemesterEditor();
    void supportsCompactScheduleEditing();
    void blocksMeetingCreationWithoutCourse();
    void rebindsReplacementViewModel();
};

void ScheduleQmlIntegrationTest::switchesResponsiveLayouts()
{
    Fixture fixture(false);
    QQmlEngine engine;
    auto page = createSchedulePage(
        engine, fixture.viewModel, fixture.managementViewModel, 640);
    QVERIFY(page);

    QCOMPARE(page->property("layoutMode").toString(), QStringLiteral("daily"));
    QTRY_VERIFY(page->findChild<QObject *>(
        QStringLiteral("dayScheduleList")) != nullptr);
    QCOMPARE(fixture.viewModel.weekDays().size(), 7);

    page->setProperty("width", 960);
    QTRY_COMPARE(
        page->property("layoutMode").toString(), QStringLiteral("grid"));
    QTRY_VERIFY(page->findChild<QObject *>(
        QStringLiteral("weeklyScheduleGrid")) != nullptr);

    page->setProperty("width", 1280);
    QTRY_COMPARE(
        page->property("layoutMode").toString(), QStringLiteral("grid"));
    QObject *grid = page->findChild<QObject *>(
        QStringLiteral("weeklyScheduleGrid"));
    QVERIFY(grid);
    QVERIFY(grid->property("width").toReal() >= 900.0);
    QCOMPARE(grid->property("periodCount").toInt(), 12);

    QObject *loader = page->findChild<QObject *>(
        QStringLiteral("scheduleLayoutLoader"));
    QVERIFY(loader);
    QTRY_VERIFY(loader->property("height").toReal() > 0.0);

}

void ScheduleQmlIntegrationTest::rendersMeetingsAndNavigatesWeeks()
{
    Fixture fixture(true);
    QQmlEngine engine;
    auto page = createSchedulePage(
        engine, fixture.viewModel, fixture.managementViewModel, 960);
    QVERIFY(page);

    QObject *grid = page->findChild<QObject *>(
        QStringLiteral("weeklyScheduleGrid"));
    QVERIFY(grid);
    QCOMPARE(grid->property("meetingCount").toInt(), 2);
    QTRY_COMPARE(grid->property("renderedMeetingCount").toInt(), 2);

    QVERIFY(grid->property("tallestCourseHeight").toReal() >= 130.0);

    fixture.viewModel.showPreviousWeek();
    QTRY_COMPARE(grid->property("meetingCount").toInt(), 3);
    QTRY_COMPARE(grid->property("renderedMeetingCount").toInt(), 3);
}

void ScheduleQmlIntegrationTest::switchesToSemesterEditor()
{
    Fixture fixture(true);
    QQmlEngine engine;
    auto page = createSchedulePage(
        engine, fixture.viewModel, fixture.managementViewModel, 960);
    QVERIFY(page);

    page->setProperty("editingSchedule", true);
    QTRY_COMPARE(
        page->property("layoutMode").toString(), QStringLiteral("editor"));
    QObject *editor = page->findChild<QObject *>(
        QStringLiteral("scheduleGridEditor"));
    QObject *grid = page->findChild<QObject *>(
        QStringLiteral("semesterScheduleGrid"));
    QVERIFY(editor);
    QVERIFY(grid);
    QCOMPARE(grid->property("editable").toBool(), true);
    QCOMPARE(grid->property("meetingCount").toInt(), 2);
    QTRY_COMPARE(grid->property("renderedMeetingCount").toInt(), 2);

    QObject *decreaseButton = page->findChild<QObject *>(
        QStringLiteral("scheduleDecreasePeriodsButton"));
    QObject *increaseButton = page->findChild<QObject *>(
        QStringLiteral("scheduleIncreasePeriodsButton"));
    QVERIFY(decreaseButton);
    QVERIFY(increaseButton);
    QVERIFY(decreaseButton->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(decreaseButton, "click"));
    QTRY_COMPARE(page->property("schedulePeriodCount").toInt(), 11);
    QTRY_COMPARE(grid->property("periodCount").toInt(), 11);
    QVERIFY(QMetaObject::invokeMethod(increaseButton, "click"));
    QTRY_COMPARE(page->property("schedulePeriodCount").toInt(), 12);
    QTRY_COMPARE(grid->property("periodCount").toInt(), 12);

    QObject *previousWeek = page->findChild<QObject *>(
        QStringLiteral("schedulePreviousWeekButton"));
    QObject *nextWeek = page->findChild<QObject *>(
        QStringLiteral("scheduleNextWeekButton"));
    QVERIFY(previousWeek);
    QVERIFY(nextWeek);
    QVERIFY(QMetaObject::invokeMethod(previousWeek, "click"));
    QTRY_COMPARE(fixture.viewModel.selectedWeek(), 1);
    QTRY_COMPARE(grid->property("meetingCount").toInt(), 3);
    QVERIFY(QMetaObject::invokeMethod(nextWeek, "click"));
    QTRY_COMPARE(fixture.viewModel.selectedWeek(), 2);
    QTRY_COMPARE(grid->property("meetingCount").toInt(), 2);

    QVERIFY(QMetaObject::invokeMethod(
        editor,
        "openNewMeeting",
        Q_ARG(QVariant, QVariant(2)),
        Q_ARG(QVariant, QVariant(3))));
    QObject *startPeriod = page->findChild<QObject *>(
        QStringLiteral("scheduleEditorStartPeriod"));
    QObject *endPeriod = page->findChild<QObject *>(
        QStringLiteral("scheduleEditorEndPeriod"));
    QObject *meetingDialog = page->findChild<QObject *>(
        QStringLiteral("scheduleGridMeetingDialog"));
    QObject *selectAllWeeks = page->findChild<QObject *>(
        QStringLiteral("scheduleSelectAllWeeksButton"));
    QObject *clearWeeks = page->findChild<QObject *>(
        QStringLiteral("scheduleClearWeeksButton"));
    QVERIFY(startPeriod);
    QVERIFY(endPeriod);
    QVERIFY(meetingDialog);
    QVERIFY(selectAllWeeks);
    QVERIFY(clearWeeks);
    QCOMPARE(startPeriod->property("value").toInt(), 3);
    QCOMPARE(endPeriod->property("value").toInt(), 4);
    const QVariantList selectedWeeks =
        meetingDialog->property("selectedWeeks").toList();
    QCOMPARE(selectedWeeks.size(), 18);
    QCOMPARE(selectedWeeks.front().toInt(), 1);
    QCOMPARE(selectedWeeks.back().toInt(), 18);
}

void ScheduleQmlIntegrationTest::supportsCompactScheduleEditing()
{
    Fixture fixture(true);
    QQmlEngine engine;
    auto page = createSchedulePage(
        engine, fixture.viewModel, fixture.managementViewModel, 360, 640);
    QVERIFY(page);

    page->setProperty("editingSchedule", true);
    QTRY_COMPARE(
        page->property("layoutMode").toString(), QStringLiteral("editor"));

    QObject *previousWeek = page->findChild<QObject *>(
        QStringLiteral("schedulePreviousWeekButton"));
    QObject *nextWeek = page->findChild<QObject *>(
        QStringLiteral("scheduleNextWeekButton"));
    QObject *editor = page->findChild<QObject *>(
        QStringLiteral("scheduleGridEditor"));
    QVERIFY(previousWeek);
    QVERIFY(nextWeek);
    QVERIFY(editor);
    QVERIFY(previousWeek->property("implicitHeight").toReal() >= 44.0);
    QVERIFY(nextWeek->property("implicitHeight").toReal() >= 44.0);

    QVERIFY(QMetaObject::invokeMethod(
        editor,
        "openNewMeeting",
        Q_ARG(QVariant, QVariant(1)),
        Q_ARG(QVariant, QVariant(0))));
    QObject *meetingDialog = page->findChild<QObject *>(
        QStringLiteral("scheduleGridMeetingDialog"));
    QObject *selectAllWeeks = page->findChild<QObject *>(
        QStringLiteral("scheduleSelectAllWeeksButton"));
    QVERIFY(meetingDialog);
    QVERIFY(selectAllWeeks);
    const qreal meetingDialogWidth =
        meetingDialog->property("width").toReal();
    QVERIFY2(
        meetingDialogWidth <= 360.0,
        qPrintable(QStringLiteral("dialog width: %1")
                       .arg(meetingDialogWidth)));
    QVERIFY(meetingDialog->property("height").toReal() <= 640.0);
    QVERIFY(selectAllWeeks->property("implicitHeight").toReal() >= 44.0);
}

void ScheduleQmlIntegrationTest::blocksMeetingCreationWithoutCourse()
{
    Fixture fixture(false, false);
    QQmlEngine engine;
    auto page = createSchedulePage(
        engine, fixture.viewModel, fixture.managementViewModel, 960);
    QVERIFY(page);

    page->setProperty("editingSchedule", true);
    QTRY_COMPARE(
        page->property("layoutMode").toString(), QStringLiteral("editor"));
    QObject *editor = page->findChild<QObject *>(
        QStringLiteral("scheduleGridEditor"));
    QObject *grid = page->findChild<QObject *>(
        QStringLiteral("semesterScheduleGrid"));
    QVERIFY(editor);
    QVERIFY(grid);
    QCOMPARE(grid->property("editable").toBool(), false);

    QVERIFY(QMetaObject::invokeMethod(
        editor,
        "openNewMeeting",
        Q_ARG(QVariant, QVariant(1)),
        Q_ARG(QVariant, QVariant(0))));
    QCOMPARE(
        editor->property("localMessage").toString(),
        QStringLiteral("请先创建并选择一门课程"));
}

void ScheduleQmlIntegrationTest::rebindsReplacementViewModel()
{
    Fixture populatedFixture(true);
    Fixture emptyFixture(false);
    QQmlEngine engine;
    auto page = createSchedulePage(
        engine,
        populatedFixture.viewModel,
        populatedFixture.managementViewModel,
        960);
    QVERIFY(page);

    QObject *grid = page->findChild<QObject *>(
        QStringLiteral("weeklyScheduleGrid"));
    QVERIFY(grid);
    QCOMPARE(grid->property("meetingCount").toInt(), 2);

    page->setProperty(
        "viewModel",
        QVariant::fromValue(
            static_cast<QObject *>(&emptyFixture.viewModel)));
    page->setProperty(
        "managementViewModel",
        QVariant::fromValue(static_cast<QObject *>(
            &emptyFixture.managementViewModel)));
    QCoreApplication::processEvents();

    QTRY_COMPARE(grid->property("meetingCount").toInt(), 0);
    QTRY_COMPARE(grid->property("renderedMeetingCount").toInt(), 0);
}

QTEST_MAIN(ScheduleQmlIntegrationTest)

#include "ScheduleQmlIntegrationTest.moc"
