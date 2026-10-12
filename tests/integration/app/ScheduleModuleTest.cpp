#include "AccountingModule.h"
#include "DataMaintenanceViewModel.h"
#include "ScheduleManagementModels.h"
#include "ScheduleManagementViewModel.h"
#include "ScheduleModule.h"
#include "ScheduleViewModel.h"

#include <QDir>
#include <QPointer>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

using namespace stukit::app;
using namespace stukit::schedule::presentation;

class ScheduleModuleTest final : public QObject
{
    Q_OBJECT

private slots:
    void persistsScheduleThroughSharedDatabase();
    void rebuildsScheduleObjectsAfterRestore();
};

void ScheduleModuleTest::persistsScheduleThroughSharedDatabase()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-schedule-module-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(
        directory.filePath(QStringLiteral("settings.ini")),
        QSettings::IniFormat);
    const QString appDataDirectory = directory.filePath(
        QStringLiteral("app-data"));

    {
        AccountingModule appModule(appDataDirectory, settings);
        QVERIFY2(
            appModule.initialize(
                QDate(2026, 9, 1), QDate(2026, 9, 8)),
            qPrintable(appModule.lastError()));
        QVERIFY(appModule.scheduleModule() != nullptr);
        auto *management = appModule.scheduleModule()->managementViewModel();
        auto *overview = appModule.scheduleModule()->overviewViewModel();
        QVERIFY(management != nullptr);
        QVERIFY(overview != nullptr);

        QVERIFY(management->saveTerm(
            {},
            QStringLiteral("2026 秋季学期"),
            QStringLiteral("2026-09-07"),
            18,
            false));
        QVERIFY(management->saveCourseDetails(
            {},
            management->selectedTermId(),
            QStringLiteral("高等数学"),
            QStringLiteral("王老师"),
            QStringLiteral("A101"),
            QStringLiteral("#3B82F6")));
        QVERIFY(management->saveMeetingDetails(
            {},
            management->selectedCourseId(),
            1,
            1,
            2,
            1,
            18,
            0));

        QCOMPARE(management->terms()->rowCount(), 1);
        QCOMPARE(management->courses()->rowCount(), 1);
        QCOMPARE(management->meetings()->rowCount(), 1);
        QCOMPARE(overview->selectedWeek(), 1);
        QCOMPARE(overview->meetings()->rowCount(), 1);
    }

    {
        AccountingModule restored(appDataDirectory, settings);
        QVERIFY2(
            restored.initialize(
                QDate(2026, 9, 1), QDate(2026, 9, 8)),
            qPrintable(restored.lastError()));
        auto *management = restored.scheduleModule()->managementViewModel();
        auto *overview = restored.scheduleModule()->overviewViewModel();
        QCOMPARE(management->terms()->rowCount(), 1);
        QCOMPARE(management->courses()->rowCount(), 1);
        QCOMPARE(management->meetings()->rowCount(), 1);
        QCOMPARE(overview->meetings()->rowCount(), 1);
        const QModelIndex courseIndex = management->courses()->index(0, 0);
        QCOMPARE(
            management->courses()->data(
                courseIndex, CourseListModel::NameRole).toString(),
            QStringLiteral("高等数学"));
    }
}

void ScheduleModuleTest::rebuildsScheduleObjectsAfterRestore()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-schedule-restore-test-XXXXXX")));
    QVERIFY(directory.isValid());
    QSettings settings(
        directory.filePath(QStringLiteral("settings.ini")),
        QSettings::IniFormat);
    AccountingModule appModule(
        directory.filePath(QStringLiteral("app-data")), settings);
    QVERIFY2(
        appModule.initialize(
            QDate(2026, 9, 1), QDate(2026, 9, 8)),
        qPrintable(appModule.lastError()));

    auto *management = appModule.scheduleModule()->managementViewModel();
    QVERIFY(management->saveTerm(
        {},
        QStringLiteral("备份学期"),
        QStringLiteral("2026-09-07"),
        18,
        false));
    const QString backupPath = directory.filePath(
        QStringLiteral("schedule-backup.sqlite"));
    DataMaintenanceViewModel maintenance(appModule);
    QVERIFY2(
        maintenance.backupTo(QUrl::fromLocalFile(backupPath)),
        qPrintable(maintenance.errorMessage()));

    QVERIFY(management->saveTerm(
        {},
        QStringLiteral("备份后新增"),
        QStringLiteral("2027-02-22"),
        18,
        false));
    QCOMPARE(management->terms()->rowCount(), 2);

    QPointer<ScheduleViewModel> oldOverview =
        appModule.scheduleModule()->overviewViewModel();
    QPointer<ScheduleManagementViewModel> oldManagement = management;
    QVERIFY2(
        maintenance.restoreFrom(QUrl::fromLocalFile(backupPath)),
        qPrintable(maintenance.errorMessage()));

    QVERIFY(oldOverview.isNull());
    QVERIFY(oldManagement.isNull());
    QVERIFY(appModule.scheduleModule() != nullptr);
    QCOMPARE(
        appModule.scheduleModule()->managementViewModel()
            ->terms()->rowCount(),
        1);
    QCOMPARE(
        appModule.scheduleModule()->managementViewModel()
            ->terms()->data(
                appModule.scheduleModule()->managementViewModel()
                    ->terms()->index(0, 0),
                AcademicTermListModel::NameRole).toString(),
        QStringLiteral("备份学期"));
}

QTEST_GUILESS_MAIN(ScheduleModuleTest)

#include "ScheduleModuleTest.moc"
