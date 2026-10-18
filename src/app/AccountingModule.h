#pragma once

#include "UserSettings.h"

#include <QDate>
#include <QString>

#include <memory>

class QSettings;

namespace stukit::accounting {
class GetMonthlyOverview;
class GetAccountBalances;
class GetAccountingAnalytics;
class GetAccountingSetup;
class CreateAccount;
class CreateCategory;
class UpdateAccount;
class UpdateCategory;
class DeleteAccountingSetupItem;
class CreateTransaction;
class DeleteTransaction;
class GetTransaction;
class GetTransactionExportData;
class UpdateTransaction;
class SqliteAccountRepository;
class SqliteCategoryRepository;
class SqliteDatabase;
class SqliteBackupService;
class SqliteTransactionRepository;

namespace presentation {
class AccountBalancesViewModel;
class AccountingAnalyticsViewModel;
class AccountingOverviewViewModel;
class AccountingSetupViewModel;
class CreateTransactionViewModel;
}
} // namespace stukit::accounting

namespace stukit::app {

class ScheduleModule;
class TaskModule;

class AccountingModule final
{
public:
    AccountingModule(QString appDataDirectory, QSettings &settings);
    ~AccountingModule();

    AccountingModule(const AccountingModule &) = delete;
    AccountingModule &operator=(const AccountingModule &) = delete;
    AccountingModule(AccountingModule &&) = delete;
    AccountingModule &operator=(AccountingModule &&) = delete;

    [[nodiscard]] bool initialize(
        const QDate &selectedMonth = QDate::currentDate(),
        const QDate &scheduleToday = QDate::currentDate());
    [[nodiscard]] bool backupDatabase(
        const QString &destinationPath,
        bool replaceExisting = false);
    [[nodiscard]] bool restoreDatabase(const QString &backupPath);
    [[nodiscard]] bool exportTransactions(
        const QString &destinationPath,
        bool replaceExisting = false);

    [[nodiscard]] accounting::presentation::AccountingOverviewViewModel *
    overviewViewModel() const noexcept;
    [[nodiscard]] accounting::presentation::AccountBalancesViewModel *
    balancesViewModel() const noexcept;
    [[nodiscard]] accounting::presentation::AccountingAnalyticsViewModel *
    analyticsViewModel() const noexcept;
    [[nodiscard]] accounting::presentation::AccountingSetupViewModel *
    setupViewModel() const noexcept;
    [[nodiscard]] accounting::presentation::CreateTransactionViewModel *
    transactionEditorViewModel() const noexcept;
    [[nodiscard]] ScheduleModule *scheduleModule() const noexcept;
    [[nodiscard]] TaskModule *taskModule() const noexcept;
    [[nodiscard]] const QString &databasePath() const noexcept;
    [[nodiscard]] const QString &lastSafetyBackupPath() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    void reset();

    QString m_appDataDirectory;
    UserSettings m_userSettings;
    QString m_databasePath;
    QString m_lastSafetyBackupPath;
    QString m_lastError;

    std::unique_ptr<ScheduleModule> m_scheduleModule;
    std::unique_ptr<TaskModule> m_taskModule;

    std::unique_ptr<accounting::SqliteDatabase> m_database;
    std::unique_ptr<accounting::SqliteBackupService> m_backupService;
    std::unique_ptr<accounting::SqliteAccountRepository> m_accountRepository;
    std::unique_ptr<accounting::SqliteCategoryRepository> m_categoryRepository;
    std::unique_ptr<accounting::SqliteTransactionRepository>
        m_transactionRepository;
    std::unique_ptr<accounting::GetMonthlyOverview> m_getMonthlyOverview;
    std::unique_ptr<accounting::GetAccountBalances> m_getAccountBalances;
    std::unique_ptr<accounting::GetAccountingAnalytics>
        m_getAccountingAnalytics;
    std::unique_ptr<accounting::GetAccountingSetup> m_getAccountingSetup;
    std::unique_ptr<accounting::CreateAccount> m_createAccount;
    std::unique_ptr<accounting::CreateCategory> m_createCategory;
    std::unique_ptr<accounting::UpdateAccount> m_updateAccount;
    std::unique_ptr<accounting::UpdateCategory> m_updateCategory;
    std::unique_ptr<accounting::DeleteAccountingSetupItem>
        m_deleteAccountingSetupItem;
    std::unique_ptr<accounting::CreateTransaction> m_createTransaction;
    std::unique_ptr<accounting::UpdateTransaction> m_updateTransaction;
    std::unique_ptr<accounting::DeleteTransaction> m_deleteTransaction;
    std::unique_ptr<accounting::GetTransaction> m_getTransaction;
    std::unique_ptr<accounting::GetTransactionExportData>
        m_getTransactionExportData;
    std::unique_ptr<accounting::presentation::AccountingOverviewViewModel>
        m_overviewViewModel;
    std::unique_ptr<accounting::presentation::AccountBalancesViewModel>
        m_balancesViewModel;
    std::unique_ptr<accounting::presentation::AccountingAnalyticsViewModel>
        m_analyticsViewModel;
    std::unique_ptr<accounting::presentation::AccountingSetupViewModel>
        m_setupViewModel;
    std::unique_ptr<accounting::presentation::CreateTransactionViewModel>
        m_transactionEditorViewModel;
};

} // namespace stukit::app
