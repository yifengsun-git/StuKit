#include "AccountingModule.h"

#include "AccountingOverviewViewModel.h"
#include "AccountingSetupViewModel.h"
#include "CreateAccount.h"
#include "CreateCategory.h"
#include "CreateTransaction.h"
#include "CreateTransactionViewModel.h"
#include "GetAccountingSetup.h"
#include "GetMonthlyOverview.h"
#include "database/SqliteDatabase.h"
#include "repositories/SqliteAccountRepository.h"
#include "repositories/SqliteCategoryRepository.h"
#include "repositories/SqliteTransactionRepository.h"

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

bool AccountingModule::initialize(const QDate &selectedMonth)
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
    m_accountRepository =
        std::make_unique<accounting::SqliteAccountRepository>(connection);
    m_categoryRepository =
        std::make_unique<accounting::SqliteCategoryRepository>(connection);
    m_transactionRepository =
        std::make_unique<accounting::SqliteTransactionRepository>(connection);
    m_getMonthlyOverview = std::make_unique<accounting::GetMonthlyOverview>(
        *m_transactionRepository);
    m_createAccount = std::make_unique<accounting::CreateAccount>(
        *m_accountRepository);
    m_createCategory = std::make_unique<accounting::CreateCategory>(
        *m_categoryRepository);
    m_getAccountingSetup = std::make_unique<accounting::GetAccountingSetup>(
        *m_accountRepository, *m_categoryRepository);
    m_createTransaction = std::make_unique<accounting::CreateTransaction>(
        *m_transactionRepository,
        *m_accountRepository,
        *m_categoryRepository);
    m_overviewViewModel = std::make_unique<
        accounting::presentation::AccountingOverviewViewModel>(
        *m_getMonthlyOverview,
        m_userSettings.accountingTimeZone(),
        selectedMonth);
    m_setupViewModel = std::make_unique<
        accounting::presentation::AccountingSetupViewModel>(
        *m_getAccountingSetup, *m_createAccount, *m_createCategory);
    m_transactionEditorViewModel = std::make_unique<
        accounting::presentation::CreateTransactionViewModel>(
        *m_createTransaction,
        *m_getAccountingSetup,
        m_overviewViewModel->timeZone());

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
        m_setupViewModel.get(),
        &accounting::presentation::AccountingSetupViewModel::setupChanged,
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::loadSetup);
    QObject::connect(
        m_transactionEditorViewModel.get(),
        &accounting::presentation::CreateTransactionViewModel::transactionCreated,
        m_overviewViewModel.get(),
        &accounting::presentation::AccountingOverviewViewModel::load);

    m_setupViewModel->load();
    m_overviewViewModel->load();
    return true;
}

accounting::presentation::CreateTransactionViewModel *
AccountingModule::transactionEditorViewModel() const noexcept
{
    return m_transactionEditorViewModel.get();
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

const QString &AccountingModule::databasePath() const noexcept
{
    return m_databasePath;
}

const QString &AccountingModule::lastError() const noexcept
{
    return m_lastError;
}

void AccountingModule::reset()
{
    m_transactionEditorViewModel.reset();
    m_setupViewModel.reset();
    m_overviewViewModel.reset();
    m_createTransaction.reset();
    m_getAccountingSetup.reset();
    m_createCategory.reset();
    m_createAccount.reset();
    m_getMonthlyOverview.reset();
    m_transactionRepository.reset();
    m_categoryRepository.reset();
    m_accountRepository.reset();
    m_database.reset();
    m_databasePath.clear();
}

} // namespace stukit::app
