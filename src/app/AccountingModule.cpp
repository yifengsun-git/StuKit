#include "AccountingModule.h"

#include "ScheduleModule.h"

#include "AccountBalancesViewModel.h"
#include "AccountingAnalyticsViewModel.h"
#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"
#include "CreateAccount.h"
#include "CreateCategory.h"
#include "CreateTransaction.h"
#include "CreateTransactionViewModel.h"
#include "DeleteTransaction.h"
#include "GetAccountingSetup.h"
#include "GetAccountingAnalytics.h"
#include "GetAccountBalances.h"
#include "GetMonthlyOverview.h"
#include "GetTransaction.h"
#include "GetTransactionExportData.h"
#include "UpdateTransaction.h"
#include "UpdateAccount.h"
#include "UpdateCategory.h"
#include "backup/SqliteBackupService.h"
#include "database/SqliteDatabase.h"
#include "export/CsvTransactionWriter.h"
#include "repositories/SqliteAccountRepository.h"
#include "repositories/SqliteCategoryRepository.h"
#include "repositories/SqliteTransactionRepository.h"

#include <QDateTime>
#include <QDir>
#include <QObject>
#include <QUuid>

#include <utility>

namespace stukit::app {

AccountingModule::AccountingModule(
    QString appDataDirectory, QSettings &settings)
    : m_appDataDirectory(std::move(appDataDirectory))
    , m_userSettings(settings)
{
}

AccountingModule::~AccountingModule() = default;

bool AccountingModule::initialize(
    const QDate &selectedMonth, const QDate &scheduleToday)
{
    reset();
    m_lastError.clear();

    if (m_appDataDirectory.trimmed().isEmpty()) {
        m_lastError = QStringLiteral("Application data directory is empty");
        return false;
    }
    if (!selectedMonth.isValid()) {
        m_lastError = QStringLiteral("Selected month is invalid");
        return false;
    }
    if (!scheduleToday.isValid()) {
        m_lastError = QStringLiteral("Schedule date is invalid");
        return false;
    }

    QDir directory;
    if (!directory.mkpath(m_appDataDirectory)) {
        m_lastError = QStringLiteral("Could not create application data directory");
        return false;
    }

    m_databasePath = QDir(m_appDataDirectory).filePath(
        QStringLiteral("stukit.sqlite"));
    const QString connectionName = QStringLiteral("stukit-app-%1").arg(
        QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_database = std::make_unique<accounting::SqliteDatabase>(connectionName);
    if (!m_database->open(m_databasePath)) {
        m_lastError = m_database->lastError();
        reset();
        return false;
    }
    if (!m_database->migrate()) {
        m_lastError = m_database->lastError();
        reset();
        return false;
    }

    const auto connection = m_database->connection();
    m_backupService = std::make_unique<accounting::SqliteBackupService>(
        connection);
    m_accountRepository =
        std::make_unique<accounting::SqliteAccountRepository>(connection);
    m_categoryRepository =
        std::make_unique<accounting::SqliteCategoryRepository>(connection);
    m_transactionRepository =
        std::make_unique<accounting::SqliteTransactionRepository>(connection);
    m_getMonthlyOverview = std::make_unique<accounting::GetMonthlyOverview>(
        *m_transactionRepository,
        *m_accountRepository,
        *m_categoryRepository);
    m_getAccountBalances = std::make_unique<accounting::GetAccountBalances>(
        *m_accountRepository, *m_transactionRepository);
    m_getAccountingAnalytics =
        std::make_unique<accounting::GetAccountingAnalytics>(
            *m_transactionRepository,
            *m_accountRepository,
            *m_categoryRepository);
    m_createAccount = std::make_unique<accounting::CreateAccount>(
        *m_accountRepository);
    m_createCategory = std::make_unique<accounting::CreateCategory>(
        *m_categoryRepository);
    m_updateAccount = std::make_unique<accounting::UpdateAccount>(
        *m_accountRepository);
    m_updateCategory = std::make_unique<accounting::UpdateCategory>(
        *m_categoryRepository);
    m_getAccountingSetup = std::make_unique<accounting::GetAccountingSetup>(
        *m_accountRepository, *m_categoryRepository);
    m_createTransaction = std::make_unique<accounting::CreateTransaction>(
        *m_transactionRepository,
        *m_accountRepository,
        *m_categoryRepository);
    m_updateTransaction = std::make_unique<accounting::UpdateTransaction>(
        *m_transactionRepository,
        *m_accountRepository,
        *m_categoryRepository);
    m_deleteTransaction = std::make_unique<accounting::DeleteTransaction>(
        *m_transactionRepository);
    m_getTransaction = std::make_unique<accounting::GetTransaction>(
        *m_transactionRepository);
    m_getTransactionExportData =
        std::make_unique<accounting::GetTransactionExportData>(
            *m_transactionRepository,
            *m_accountRepository,
            *m_categoryRepository);
    m_overviewViewModel = std::make_unique<
        accounting::presentation::AccountingOverviewViewModel>(
        *m_getMonthlyOverview,
        m_userSettings.accountingTimeZone(),
        selectedMonth);
    m_balancesViewModel = std::make_unique<
        accounting::presentation::AccountBalancesViewModel>(
        *m_getAccountBalances);
    m_analyticsViewModel = std::make_unique<
        accounting::presentation::AccountingAnalyticsViewModel>(
        *m_getAccountingAnalytics,
        *m_getAccountingSetup,
        m_userSettings.accountingTimeZone(),
        selectedMonth);
    m_setupViewModel = std::make_unique<
        accounting::presentation::AccountingSetupViewModel>(
        *m_getAccountingSetup,
        *m_createAccount,
        *m_createCategory,
        *m_updateAccount,
        *m_updateCategory);
    m_transactionEditorViewModel = std::make_unique<
        accounting::presentation::CreateTransactionViewModel>(
        *m_createTransaction,
        *m_updateTransaction,
        *m_deleteTransaction,
        *m_getTransaction,
        *m_getAccountingSetup,
        m_overviewViewModel->timeZone());
    m_scheduleModule = std::make_unique<ScheduleModule>(connection);
    if (!m_scheduleModule->initialize(scheduleToday)) {
        m_lastError = m_scheduleModule->lastError();
        reset();
        return false;
    }

    QObject::connect(
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::timeZoneChanged,
        m_overviewViewModel.get(),
        [this]() {
            m_userSettings.setAccountingTimeZone(
                m_overviewViewModel->timeZone());
        });
    QObject::connect(
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::timeZoneChanged,
        m_transactionEditorViewModel.get(),
        [this]() {
            m_transactionEditorViewModel->setTimeZone(
                m_overviewViewModel->timeZone());
        });
    QObject::connect(
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::timeZoneChanged,
        m_analyticsViewModel.get(),
        [this]() {
            m_analyticsViewModel->setTimeZone(
                m_overviewViewModel->timeZone());
        });
    QObject::connect(
        m_setupViewModel.get(),
        &accounting::presentation::AccountingSetupViewModel::setupChanged,
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::loadSetup);
    QObject::connect(
        m_setupViewModel.get(),
        &accounting::presentation::AccountingSetupViewModel::setupChanged,
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::load);
    QObject::connect(
        m_setupViewModel.get(),
        &accounting::presentation::AccountingSetupViewModel::setupChanged,
        m_balancesViewModel.get(),
        &accounting::presentation::AccountBalancesViewModel::load);
    QObject::connect(
        m_setupViewModel.get(),
        &accounting::presentation::AccountingSetupViewModel::setupChanged,
        m_analyticsViewModel.get(),
        &accounting::presentation::AccountingAnalyticsViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionCreated,
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionCreated,
        m_balancesViewModel.get(),
        &accounting::presentation::AccountBalancesViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionCreated,
        m_analyticsViewModel.get(),
        &accounting::presentation::AccountingAnalyticsViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionUpdated,
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionUpdated,
        m_balancesViewModel.get(),
        &accounting::presentation::AccountBalancesViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionUpdated,
        m_analyticsViewModel.get(),
        &accounting::presentation::AccountingAnalyticsViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionDeleted,
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionDeleted,
        m_balancesViewModel.get(),
        &accounting::presentation::AccountBalancesViewModel::load);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionDeleted,
        m_analyticsViewModel.get(),
        &accounting::presentation::AccountingAnalyticsViewModel::load);

    m_setupViewModel->load();
    return true;
}

bool AccountingModule::backupDatabase(
    const QString &destinationPath,
    bool replaceExisting)
{
    m_lastError.clear();
    if (!m_backupService) {
        m_lastError = QStringLiteral("Database backup service is unavailable");
        return false;
    }

    const accounting::SqliteBackupResult result =
        m_backupService->createBackup(destinationPath, replaceExisting);
    if (!result.succeeded()) {
        m_lastError = result.detail;
        return false;
    }
    return true;
}

bool AccountingModule::restoreDatabase(const QString &backupPath)
{
    m_lastError.clear();
    if (!m_backupService || !m_overviewViewModel) {
        m_lastError = QStringLiteral("Accounting module is not initialized");
        return false;
    }

    const accounting::SqliteBackupResult validation =
        accounting::SqliteBackupService::validateBackup(backupPath);
    if (!validation.succeeded()) {
        m_lastError = validation.detail;
        return false;
    }

    const QDate selectedMonth(
        m_overviewViewModel->selectedYear(),
        m_overviewViewModel->selectedMonth(),
        1);
    const QString activeDatabasePath = m_databasePath;
    const QDate scheduleToday = m_scheduleModule != nullptr
        ? m_scheduleModule->today()
        : QDate::currentDate();
    const QString safetyDirectory = QDir(m_appDataDirectory).filePath(
        QStringLiteral("backups"));
    if (!QDir().mkpath(safetyDirectory)) {
        m_lastError = QStringLiteral("Could not create safety backup directory");
        return false;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString safetyPath = QDir(safetyDirectory).filePath(
        QStringLiteral("pre-restore-%1-%2.sqlite")
            .arg(timestamp,
                 QUuid::createUuid().toString(QUuid::WithoutBraces)));
    const accounting::SqliteBackupResult safetyResult =
        m_backupService->createBackup(safetyPath);
    if (!safetyResult.succeeded()) {
        m_lastError = QStringLiteral("Could not create safety backup: %1")
            .arg(safetyResult.detail);
        return false;
    }
    m_lastSafetyBackupPath = safetyPath;

    reset();
    const accounting::SqliteBackupResult replacement =
        accounting::SqliteBackupService::replaceDatabase(
            backupPath, activeDatabasePath);
    if (!replacement.succeeded()) {
        const bool reopened = initialize(selectedMonth, scheduleToday);
        const QString reopenDetail = reopened
            ? QString{}
            : QStringLiteral("; reopening current data failed: %1")
                  .arg(m_lastError);
        m_lastError = QStringLiteral("Could not install backup: %1%2")
            .arg(replacement.detail, reopenDetail);
        return false;
    }

    if (initialize(selectedMonth, scheduleToday)) {
        return true;
    }

    const QString restoredDatabaseError = m_lastError;
    reset();
    const accounting::SqliteBackupResult rollback =
        accounting::SqliteBackupService::replaceDatabase(
            safetyPath, activeDatabasePath);
    if (!rollback.succeeded()) {
        m_lastError = QStringLiteral(
            "Restored database failed to initialize (%1); rollback failed (%2)")
            .arg(restoredDatabaseError, rollback.detail);
        return false;
    }

    const bool rollbackInitialized = initialize(
        selectedMonth, scheduleToday);
    const QString rollbackInitializationError = m_lastError;
    m_lastError = rollbackInitialized
        ? QStringLiteral(
              "Restored database failed to initialize; original data was restored: %1")
              .arg(restoredDatabaseError)
        : QStringLiteral(
              "Restored database failed to initialize (%1); "
              "original data also failed to reopen (%2)")
              .arg(restoredDatabaseError, rollbackInitializationError);
    return false;
}

bool AccountingModule::exportTransactions(
    const QString &destinationPath,
    bool replaceExisting)
{
    m_lastError.clear();
    if (!m_getTransactionExportData) {
        m_lastError = QStringLiteral("Accounting module is not initialized");
        return false;
    }

    const accounting::GetTransactionExportDataResult exportDataResult =
        m_getTransactionExportData->execute();
    if (!std::holds_alternative<accounting::TransactionExportData>(
            exportDataResult)) {
        m_lastError = QStringLiteral("Could not read transactions for export");
        return false;
    }

    const accounting::CsvExportResult writeResult =
        accounting::CsvTransactionWriter::write(
            destinationPath,
            std::get<accounting::TransactionExportData>(exportDataResult),
            m_userSettings.accountingTimeZone(),
            replaceExisting);
    if (!writeResult.succeeded()) {
        m_lastError = writeResult.detail;
        return false;
    }
    return true;
}

accounting::presentation::CreateTransactionViewModel *
AccountingModule::transactionEditorViewModel() const noexcept
{
    return m_transactionEditorViewModel.get();
}

ScheduleModule *AccountingModule::scheduleModule() const noexcept
{
    return m_scheduleModule.get();
}

accounting::presentation::AccountingSetupViewModel *
AccountingModule::setupViewModel() const noexcept
{
    return m_setupViewModel.get();
}

accounting::presentation::AccountingOverviewViewModel *
AccountingModule::overviewViewModel() const noexcept
{
    return m_overviewViewModel.get();
}

accounting::presentation::AccountBalancesViewModel *
AccountingModule::balancesViewModel() const noexcept
{
    return m_balancesViewModel.get();
}

accounting::presentation::AccountingAnalyticsViewModel *
AccountingModule::analyticsViewModel() const noexcept
{
    return m_analyticsViewModel.get();
}

const QString &AccountingModule::databasePath() const noexcept
{
    return m_databasePath;
}

const QString &AccountingModule::lastSafetyBackupPath() const noexcept
{
    return m_lastSafetyBackupPath;
}

const QString &AccountingModule::lastError() const noexcept
{
    return m_lastError;
}

void AccountingModule::reset()
{
    m_scheduleModule.reset();
    m_transactionEditorViewModel.reset();
    m_analyticsViewModel.reset();
    m_setupViewModel.reset();
    m_balancesViewModel.reset();
    m_overviewViewModel.reset();
    m_getTransactionExportData.reset();
    m_getTransaction.reset();
    m_deleteTransaction.reset();
    m_updateTransaction.reset();
    m_createTransaction.reset();
    m_getAccountingSetup.reset();
    m_getAccountingAnalytics.reset();
    m_getAccountBalances.reset();
    m_updateCategory.reset();
    m_updateAccount.reset();
    m_createCategory.reset();
    m_createAccount.reset();
    m_getMonthlyOverview.reset();
    m_transactionRepository.reset();
    m_categoryRepository.reset();
    m_accountRepository.reset();
    m_backupService.reset();
    m_database.reset();
    m_databasePath.clear();
}

} // namespace stukit::app
