#pragma once

#include "UserSettings.h"

#include <QDate>
#include <QString>

#include <memory>

class QSettings;

namespace stukit::accounting {
class GetMonthlyOverview;
class SqliteAccountRepository;
class SqliteCategoryRepository;
class SqliteDatabase;
class SqliteTransactionRepository;

namespace presentation {
class AccountingOverviewViewModel;
}
} // namespace stukit::accounting

namespace stukit::app {

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
        const QDate &selectedMonth = QDate::currentDate());

    [[nodiscard]] accounting::presentation::AccountingOverviewViewModel *
    overviewViewModel() const noexcept;
    [[nodiscard]] const QString &databasePath() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    void reset();

    QString m_appDataDirectory;
    UserSettings m_userSettings;
    QString m_databasePath;
    QString m_lastError;

    std::unique_ptr<accounting::SqliteDatabase> m_database;
    std::unique_ptr<accounting::SqliteAccountRepository> m_accountRepository;
    std::unique_ptr<accounting::SqliteCategoryRepository> m_categoryRepository;
    std::unique_ptr<accounting::SqliteTransactionRepository>
        m_transactionRepository;
    std::unique_ptr<accounting::GetMonthlyOverview> m_getMonthlyOverview;
    std::unique_ptr<accounting::presentation::AccountingOverviewViewModel>
        m_overviewViewModel;
};

} // namespace stukit::app
