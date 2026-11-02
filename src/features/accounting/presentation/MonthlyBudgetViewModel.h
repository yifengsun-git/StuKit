#pragma once

#include "ManageMonthlyBudgets.h"

#include <QDate>
#include <QObject>
#include <QString>
#include <QTimeZone>
#include <QVariantList>

namespace stukit::accounting::presentation {

class MonthlyBudgetViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString monthLabel READ monthLabel NOTIFY selectedMonthChanged)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY budgetsChanged)
    Q_PROPERTY(bool hasCategories READ hasCategories NOTIFY budgetsChanged)
    Q_PROPERTY(bool hasBudgets READ hasBudgets NOTIFY budgetsChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    MonthlyBudgetViewModel(
        ManageMonthlyBudgets &manageBudgets,
        QTimeZone timeZone,
        QDate selectedMonth,
        QObject *parent = nullptr);

    [[nodiscard]] QString monthLabel() const;
    [[nodiscard]] const QVariantList &categories() const noexcept;
    [[nodiscard]] bool hasCategories() const noexcept;
    [[nodiscard]] bool hasBudgets() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;

    void setSelectedMonth(int year, int month);
    void setTimeZone(QTimeZone timeZone);

    Q_INVOKABLE void load();
    Q_INVOKABLE bool saveBudget(
        const QString &categoryId, const QString &amountText);
    Q_INVOKABLE bool removeBudget(const QString &categoryId);

signals:
    void selectedMonthChanged();
    void budgetsChanged();
    void errorChanged();

private:
    void setError(QString message);
    void clearError();
    void rebuildCategories(const MonthlyBudgetOverview &overview);

    ManageMonthlyBudgets &m_manageBudgets;
    QTimeZone m_timeZone;
    QDate m_selectedMonth;
    QVariantList m_categories;
    QString m_errorMessage;
};

} // namespace stukit::accounting::presentation
