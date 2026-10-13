#include "AccountingModule.h"
#include "AccountBalancesViewModel.h"
#include "AccountingAnalyticsViewModel.h"
#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"
#include "CreateTransactionViewModel.h"
#include "DataMaintenanceViewModel.h"
#include "ScheduleManagementViewModel.h"
#include "ScheduleModule.h"
#include "ScheduleViewModel.h"
#include "TaskModule.h"
#include "TasksViewModel.h"
#include "WeatherModule.h"
#include "WeatherViewModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>
#include <QVariantMap>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("StuKit");
    QCoreApplication::setApplicationName("StuKit");

    QString appDataDirectory = qEnvironmentVariable("STUKIT_APP_DATA_DIR");
    if (appDataDirectory.trimmed().isEmpty()) {
        appDataDirectory = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
    }

    QSettings settings;
    stukit::app::AccountingModule accountingModule(appDataDirectory, settings);
    stukit::app::WeatherModule weatherModule(appDataDirectory, settings);
    stukit::app::DataMaintenanceViewModel dataMaintenance(accountingModule);

    QString accountingStartupError;
    if (!accountingModule.initialize()) {
        qCritical().noquote()
            << "Accounting module initialization failed:"
            << accountingModule.lastError();
        accountingStartupError = QStringLiteral("记账数据暂时无法打开");
    }
    if (!weatherModule.initialize()) {
        qCritical().noquote()
            << "Weather module initialization failed:"
            << weatherModule.lastError();
    }

    QQmlApplicationEngine engine;
    QVariantMap initialProperties;
    initialProperties.insert(
        QStringLiteral("accountingOverview"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.overviewViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingBalances"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.balancesViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingAnalytics"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.analyticsViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingSetup"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.setupViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingTransactionEditor"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.transactionEditorViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingStartupError"), accountingStartupError);
    initialProperties.insert(
        QStringLiteral("dataMaintenance"),
        QVariant::fromValue(static_cast<QObject *>(&dataMaintenance)));
    initialProperties.insert(
        QStringLiteral("scheduleOverview"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.scheduleModule() != nullptr
                ? accountingModule.scheduleModule()->overviewViewModel()
                : nullptr)));
    initialProperties.insert(
        QStringLiteral("scheduleManagement"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.scheduleModule() != nullptr
                ? accountingModule.scheduleModule()->managementViewModel()
                : nullptr)));
    initialProperties.insert(
        QStringLiteral("scheduleStartupError"),
        accountingModule.scheduleModule() == nullptr
            ? QStringLiteral("课表数据暂时无法打开")
            : QString{});
    initialProperties.insert(
        QStringLiteral("weatherViewModel"),
        QVariant::fromValue(static_cast<QObject *>(weatherModule.viewModel())));
    initialProperties.insert(
        QStringLiteral("tasksViewModel"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.taskModule() != nullptr
                ? accountingModule.taskModule()->viewModel() : nullptr)));
    engine.setInitialProperties(initialProperties);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("StuKit", "Main");

    if (!engine.rootObjects().isEmpty()) {
        QObject *rootObject = engine.rootObjects().constFirst();
        QObject::connect(
            &dataMaintenance,
            &stukit::app::DataMaintenanceViewModel::accountingObjectsChanged,
            rootObject,
            [rootObject, &accountingModule]() {
                rootObject->setProperty(
                    "accountingOverview",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.overviewViewModel())));
                rootObject->setProperty(
                    "accountingBalances",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.balancesViewModel())));
                rootObject->setProperty(
                    "accountingAnalytics",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.analyticsViewModel())));
                rootObject->setProperty(
                    "accountingSetup",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.setupViewModel())));
                rootObject->setProperty(
                    "accountingTransactionEditor",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.transactionEditorViewModel())));
                rootObject->setProperty(
                    "scheduleOverview",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.scheduleModule() != nullptr
                            ? accountingModule.scheduleModule()
                                  ->overviewViewModel()
                            : nullptr)));
                rootObject->setProperty(
                    "scheduleManagement",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.scheduleModule() != nullptr
                            ? accountingModule.scheduleModule()
                                  ->managementViewModel()
                            : nullptr)));
                rootObject->setProperty(
                    "scheduleStartupError",
                    accountingModule.scheduleModule() == nullptr
                        ? QStringLiteral("课表数据暂时无法打开")
                        : QString{});
                rootObject->setProperty(
                    "tasksViewModel",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.taskModule() != nullptr
                            ? accountingModule.taskModule()->viewModel()
                            : nullptr)));
            });
    }

    return app.exec();
}
