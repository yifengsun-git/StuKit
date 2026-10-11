#pragma once

#include "GetMonthlyOverview.h"
#include "TransactionFilterProxyModel.h"
#include "TransactionListModel.h"

#include <QAbstractItemModel>
#include <QDate>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimeZone>
#include <QVariantList>

#include <vector>

namespace stukit::accounting::presentation {

class AccountingOverviewViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY selectedMonthChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth NOTIFY selectedMonthChanged)
    Q_PROPERTY(QString monthLabel READ monthLabel NOTIFY selectedMonthChanged)
    Q_PROPERTY(QString timeZoneId READ timeZoneId NOTIFY timeZoneChanged)
    Q_PROPERTY(QStringList availableTimeZoneIds READ availableTimeZoneIds CONSTANT)
    Q_PROPERTY(QString totalIncomeText READ totalIncomeText NOTIFY overviewChanged)
    Q_PROPERTY(QString totalExpenseText READ totalExpenseText NOTIFY overviewChanged)
    Q_PROPERTY(QString balanceText READ balanceText NOTIFY overviewChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY overviewChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QAbstractItemModel *transactions READ transactions CONSTANT)
    Q_PROPERTY(int typeFilter READ typeFilter WRITE setTypeFilter NOTIFY transactionFiltersChanged)
    Q_PROPERTY(QString accountIdFilter READ accountIdFilter WRITE setAccountIdFilter NOTIFY transactionFiltersChanged)
    Q_PROPERTY(QString categoryIdFilter READ categoryIdFilter WRITE setCategoryIdFilter NOTIFY transactionFiltersChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY transactionFiltersChanged)
    Q_PROPERTY(QVariantList accountFilterOptions READ accountFilterOptions NOTIFY transactionFiltersChanged)
    Q_PROPERTY(QVariantList categoryFilterOptions READ categoryFilterOptions NOTIFY transactionFiltersChanged)
    Q_PROPERTY(int filteredTransactionCount READ filteredTransactionCount NOTIFY transactionFiltersChanged)
    Q_PROPERTY(bool hasActiveTransactionFilters READ hasActiveTransactionFilters NOTIFY transactionFiltersChanged)
    Q_PROPERTY(bool noMatchingTransactions READ noMatchingTransactions NOTIFY transactionFiltersChanged)

public:
    AccountingOverviewViewModel(
        GetMonthlyOverview &getMonthlyOverview,
        QTimeZone timeZone,
        QDate selectedMonth,
        QObject *parent = nullptr);

    [[nodiscard]] int selectedYear() const noexcept;
    [[nodiscard]] int selectedMonth() const noexcept;
    [[nodiscard]] QString monthLabel() const;
    [[nodiscard]] QString timeZoneId() const;
    [[nodiscard]] QStringList availableTimeZoneIds() const;
    [[nodiscard]] const QTimeZone &timeZone() const noexcept;
    [[nodiscard]] const QString &totalIncomeText() const noexcept;
    [[nodiscard]] const QString &totalExpenseText() const noexcept;
    [[nodiscard]] const QString &balanceText() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] QAbstractItemModel *transactions() noexcept;
    [[nodiscard]] int typeFilter() const noexcept;
    [[nodiscard]] const QString &accountIdFilter() const noexcept;
    [[nodiscard]] const QString &categoryIdFilter() const noexcept;
    [[nodiscard]] const QString &searchText() const noexcept;
    [[nodiscard]] QVariantList accountFilterOptions() const;
    [[nodiscard]] QVariantList categoryFilterOptions() const;
    [[nodiscard]] int filteredTransactionCount() const noexcept;
    [[nodiscard]] bool hasActiveTransactionFilters() const noexcept;
    [[nodiscard]] bool noMatchingTransactions() const noexcept;

    void setTypeFilter(int filter);
    void setAccountIdFilter(const QString &accountId);
    void setCategoryIdFilter(const QString &categoryId);
    void setSearchText(const QString &searchText);

    Q_INVOKABLE void load();
    Q_INVOKABLE void showPreviousMonth();
    Q_INVOKABLE void showNextMonth();
    Q_INVOKABLE bool selectTimeZone(const QString &timeZoneId);
    Q_INVOKABLE bool selectFixedOffsetMinutes(int offsetMinutes);
    Q_INVOKABLE void clearTransactionFilters();

signals:
    void selectedMonthChanged();
    void timeZoneChanged();
    void overviewChanged();
    void errorChanged();
    void transactionFiltersChanged();

private:
    struct NamedFilterOption
    {
        QString id;
        QString name;
    };

    struct CategoryFilterOption
    {
        QString id;
        QString name;
        bool income{false};
    };

    void changeMonth(const QDate &month);
    bool applyTimeZone(const QTimeZone &timeZone);
    void rebuildFilterOptions(
        const std::vector<MonthlyTransactionItem> &transactions);
    [[nodiscard]] bool categoryMatchesType(
        const QString &categoryId,
        TransactionFilterProxyModel::TypeFilter filter) const;
    void setError(QString message);
    void clearError();

    GetMonthlyOverview &m_getMonthlyOverview;
    QTimeZone m_timeZone;
    QDate m_selectedMonth;
    TransactionListModel m_transactions;
    TransactionFilterProxyModel m_filteredTransactions;
    std::vector<NamedFilterOption> m_accountFilterOptions;
    std::vector<CategoryFilterOption> m_categoryFilterOptions;
    QString m_totalIncomeText{QStringLiteral("0.00")};
    QString m_totalExpenseText{QStringLiteral("0.00")};
    QString m_balanceText{QStringLiteral("0.00")};
    QString m_errorMessage;
};

} // namespace stukit::accounting::presentation
