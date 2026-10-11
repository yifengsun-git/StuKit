#pragma once

#include "AccountAnalyticsListModel.h"
#include "CategoryAnalyticsListModel.h"
#include "ChartSeriesListModel.h"
#include "GetAccountingAnalytics.h"
#include "GetAccountingSetup.h"
#include "MonthlyAnalyticsListModel.h"

#include <QAbstractItemModel>
#include <QDate>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimeZone>
#include <QVariantList>

namespace stukit::accounting::presentation {

class AccountingAnalyticsViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY selectedMonthChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth NOTIFY selectedMonthChanged)
    Q_PROPERTY(QString monthLabel READ monthLabel NOTIFY selectedMonthChanged)
    Q_PROPERTY(int monthCount READ monthCount WRITE setMonthCount NOTIFY filtersChanged)
    Q_PROPERTY(QString timeZoneId READ timeZoneId NOTIFY timeZoneChanged)
    Q_PROPERTY(QStringList selectedAccountIds READ selectedAccountIds NOTIFY filtersChanged)
    Q_PROPERTY(QStringList selectedCategoryIds READ selectedCategoryIds NOTIFY filtersChanged)
    Q_PROPERTY(QVariantList accountFilterOptions READ accountFilterOptions NOTIFY filtersChanged)
    Q_PROPERTY(QVariantList categoryFilterOptions READ categoryFilterOptions NOTIFY filtersChanged)
    Q_PROPERTY(QAbstractItemModel *months READ months CONSTANT)
    Q_PROPERTY(QAbstractItemModel *incomeCategories READ incomeCategories CONSTANT)
    Q_PROPERTY(QAbstractItemModel *expenseCategories READ expenseCategories CONSTANT)
    Q_PROPERTY(QAbstractItemModel *accountFlows READ accountFlows CONSTANT)
    Q_PROPERTY(QAbstractItemModel *trendSeries READ trendSeries CONSTANT)
    Q_PROPERTY(QString selectedIncomeText READ selectedIncomeText NOTIFY analyticsChanged)
    Q_PROPERTY(QString selectedExpenseText READ selectedExpenseText NOTIFY analyticsChanged)
    Q_PROPERTY(QString selectedBalanceText READ selectedBalanceText NOTIFY analyticsChanged)
    Q_PROPERTY(QString incomeChangeText READ incomeChangeText NOTIFY analyticsChanged)
    Q_PROPERTY(QString expenseChangeText READ expenseChangeText NOTIFY analyticsChanged)
    Q_PROPERTY(QString balanceChangeText READ balanceChangeText NOTIFY analyticsChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY analyticsChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    AccountingAnalyticsViewModel(
        GetAccountingAnalytics &getAccountingAnalytics,
        GetAccountingSetup &getAccountingSetup,
        QTimeZone timeZone,
        QDate selectedMonth,
        QObject *parent = nullptr);

    [[nodiscard]] int selectedYear() const noexcept;
    [[nodiscard]] int selectedMonth() const noexcept;
    [[nodiscard]] QString monthLabel() const;
    [[nodiscard]] int monthCount() const noexcept;
    [[nodiscard]] QString timeZoneId() const;
    [[nodiscard]] QStringList selectedAccountIds() const;
    [[nodiscard]] QStringList selectedCategoryIds() const;
    [[nodiscard]] const QVariantList &accountFilterOptions() const noexcept;
    [[nodiscard]] const QVariantList &categoryFilterOptions() const noexcept;
    [[nodiscard]] QAbstractItemModel *months() noexcept;
    [[nodiscard]] QAbstractItemModel *incomeCategories() noexcept;
    [[nodiscard]] QAbstractItemModel *expenseCategories() noexcept;
    [[nodiscard]] QAbstractItemModel *accountFlows() noexcept;
    [[nodiscard]] QAbstractItemModel *trendSeries() noexcept;
    [[nodiscard]] const QString &selectedIncomeText() const noexcept;
    [[nodiscard]] const QString &selectedExpenseText() const noexcept;
    [[nodiscard]] const QString &selectedBalanceText() const noexcept;
    [[nodiscard]] const QString &incomeChangeText() const noexcept;
    [[nodiscard]] const QString &expenseChangeText() const noexcept;
    [[nodiscard]] const QString &balanceChangeText() const noexcept;
    [[nodiscard]] bool hasData() const noexcept;
    [[nodiscard]] bool isLoading() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] const QTimeZone &timeZone() const noexcept;

    void setMonthCount(int count);
    void setTimeZone(const QTimeZone &timeZone);

    Q_INVOKABLE void load();
    Q_INVOKABLE void showPreviousMonth();
    Q_INVOKABLE void showNextMonth();
    Q_INVOKABLE bool setAccountSelected(
        const QString &accountId, bool selected);
    Q_INVOKABLE bool setCategorySelected(
        const QString &categoryId, bool selected);
    Q_INVOKABLE void resetFilters();

signals:
    void selectedMonthChanged();
    void timeZoneChanged();
    void filtersChanged();
    void analyticsChanged();
    void loadingChanged();
    void errorChanged();

private:
    void changeMonth(const QDate &month);
    void rebuildFilterOptions(const AccountingSetup &setup);
    void applyAnalytics(AccountingAnalytics analytics);
    void clearAnalytics();
    void setLoading(bool loading);
    void setError(QString message);
    void clearError();

    GetAccountingAnalytics &m_getAccountingAnalytics;
    GetAccountingSetup &m_getAccountingSetup;
    QTimeZone m_timeZone;
    QDate m_defaultSelectedMonth;
    QDate m_selectedMonth;
    int m_monthCount{12};
    QSet<QUuid> m_availableAccountIds;
    QSet<QUuid> m_availableCategoryIds;
    QSet<QUuid> m_selectedAccountIds;
    QSet<QUuid> m_selectedCategoryIds;
    QVariantList m_accountFilterOptions;
    QVariantList m_categoryFilterOptions;
    MonthlyAnalyticsListModel m_months;
    CategoryAnalyticsListModel m_incomeCategories;
    CategoryAnalyticsListModel m_expenseCategories;
    AccountAnalyticsListModel m_accountFlows;
    analytics::presentation::ChartSeriesListModel m_trendSeries;
    QString m_selectedIncomeText{QStringLiteral("0.00")};
    QString m_selectedExpenseText{QStringLiteral("0.00")};
    QString m_selectedBalanceText{QStringLiteral("0.00")};
    QString m_incomeChangeText{QStringLiteral("0.00")};
    QString m_expenseChangeText{QStringLiteral("0.00")};
    QString m_balanceChangeText{QStringLiteral("0.00")};
    QString m_errorMessage;
    bool m_hasData{false};
    bool m_loading{false};
};

} // namespace stukit::accounting::presentation
