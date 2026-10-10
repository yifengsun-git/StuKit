#include "AccountingModule.h"
#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>
#include <QVariantMap>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("StuKit");
    QCoreApplication::setApplicationName("StuKit");

    QString appDataDirectory = qEnvironmentVariable("STUKIT_APP_DATA_DIR");
    if (appDataDirectory.trimmed().isEmpty()) {
        appDataDirectory = QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation);
    }

    QSettings settings;
    stukit::app::AccountingModule accountingModule(appDataDirectory, settings);

    QString accountingStartupError;
    if (!accountingModule.initialize()) {
        qCritical().noquote()
            << "Accounting module initialization failed:"
            << accountingModule.lastError();
        accountingStartupError = QStringLiteral("记账数据暂时无法打开");
    }

    QQmlApplicationEngine engine;
    QVariantMap initialProperties;
    initialProperties.insert(
        QStringLiteral("accountingOverview"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.overviewViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingSetup"),
        QVariant::fromValue(static_cast<QObject *>(
            accountingModule.setupViewModel())));
    initialProperties.insert(
        QStringLiteral("accountingStartupError"), accountingStartupError);
    engine.setInitialProperties(initialProperties);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("StuKit", "Main");

    return app.exec();
}
