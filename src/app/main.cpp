#include "AccountingModule.h"
#include "AccountBalancesViewModel.h"
#include "AccountingAnalyticsViewModel.h"
#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"
#include "AppearanceSettingsViewModel.h"
#include "ApplicationStorage.h"
#include "CreateTransactionViewModel.h"
#include "TransferFundsViewModel.h"
#include "MonthlyBudgetViewModel.h"
#include "DataMaintenanceViewModel.h"
#include "ScheduleManagementViewModel.h"
#include "ScheduleModule.h"
#include "ScheduleViewModel.h"
#include "TaskModule.h"
#include "TasksViewModel.h"
#include "UpdateModule.h"
#include "WeatherModule.h"
#include "WeatherViewModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>
#include <QWindow>

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Basic"));
    }

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/stukit.png")));

    QCoreApplication::setOrganizationName("StuKit");
    QCoreApplication::setApplicationName("StuKit");
    QCoreApplication::setApplicationVersion(QStringLiteral(STUKIT_VERSION));

    const QString environmentDataDirectory = qEnvironmentVariable(
        "STUKIT_APP_DATA_DIR");
    const auto storageLocation = stukit::app::ApplicationStorage::resolve(
        QCoreApplication::applicationDirPath(),
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        environmentDataDirectory);
    QString storageError;
    if (!stukit::app::ApplicationStorage::prepareDataDirectory(
            storageLocation, storageError)) {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("StuKit 数据目录错误"),
            storageError);
        return 1;
    }
    const QString appDataDirectory = storageLocation.dataDirectory;
    QSettings settings(
        stukit::app::ApplicationStorage::settingsPath(appDataDirectory),
        QSettings::IniFormat);
    if (storageLocation.migrateLegacySettings) {
        QSettings legacySettings;
        if (!stukit::app::ApplicationStorage::migrateSettings(
                legacySettings, settings, storageError)) {
            QMessageBox::critical(
                nullptr,
                QStringLiteral("StuKit 设置迁移失败"),
                storageError);
            return 1;
        }
    }

    stukit::app::AppearanceSettingsViewModel appearanceSettings(
        settings,
        QDir(appDataDirectory).filePath(QStringLiteral("themes")));
    stukit::app::AccountingModule accountingModule(appDataDirectory, settings);
    stukit::app::WeatherModule weatherModule(appDataDirectory, settings);
    stukit::app::UpdateModule updateModule(
        accountingModule,
        appDataDirectory,
        QCoreApplication::applicationVersion());
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
    QStringList qmlLoadErrors;
    QObject::connect(
        &engine,
        &QQmlEngine::warnings,
        &app,
        [&qmlLoadErrors](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings) {
                qmlLoadErrors.push_back(warning.toString());
            }
        });
    QVariantMap initialProperties;
    initialProperties.insert(
        QStringLiteral("appearanceSettings"),
        QVariant::fromValue(static_cast<QObject *>(&appearanceSettings)));
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
        QStringLiteral("accountingTransferEditor"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.transferEditorViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingBudget"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.budgetViewModel())));
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
    initialProperties.insert(
        QStringLiteral("updateViewModel"),
        QVariant::fromValue(static_cast<QObject *>(updateModule.viewModel())));
    engine.setInitialProperties(initialProperties);

    if (accountingModule.taskModule() != nullptr) {
        auto *tasksViewModel = accountingModule.taskModule()->viewModel();
        QObject::connect(
            &app,
            &QGuiApplication::applicationStateChanged,
            tasksViewModel,
            [tasksViewModel](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive) {
                    tasksViewModel->scanReminders();
                }
            });
    }

    engine.loadFromModule("StuKit", "Main");

    if (engine.rootObjects().isEmpty()) {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("StuKit 界面加载失败"),
            qmlLoadErrors.isEmpty()
                ? QStringLiteral("主界面无法创建，请重新构建后再试。")
                : qmlLoadErrors.join(QStringLiteral("\n")));
        return -1;
    }

    {
        QObject *rootObject = engine.rootObjects().constFirst();
        if (auto *rootWindow = qobject_cast<QWindow *>(rootObject)) {
            rootWindow->show();
            QTimer::singleShot(0, rootWindow, [rootWindow]() {
                rootWindow->raise();
                rootWindow->requestActivate();
            });
        }
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
                    "accountingTransferEditor",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.transferEditorViewModel())));
                rootObject->setProperty(
                    "accountingBudget",
                    QVariant::fromValue(static_cast<QObject *>(
                        accountingModule.budgetViewModel())));
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
