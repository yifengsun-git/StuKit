#pragma once

#include "GetMonthlyOverview.h"
#include "TransactionListModel.h"

#include <QAbstractItemModel>
#include <QDate>
#include <QObject>
#include <QString>
#include <QTimeZone>

namespace stukit::accounting::presentation {

class AccountingOverviewViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY selectedMonthChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth NOTIFY selectedMonthChanged)
    Q_PROPERTY(QString monthLabel READ monthLabel NOTIFY selectedMonthChanged)
    Q_PROPERTY(QString timeZoneId READ timeZoneId NOTIFY timeZoneChanged)
    Q_PROPERTY(QString totalIncomeText READ totalIncomeText NOTIFY overviewChanged)
    Q_PROPERTY(QString totalExpenseText READ totalExpenseText NOTIFY overviewChanged)
    Q_PROPERTY(QString balanceText READ balanceText NOTIFY overviewChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY overviewChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QAbstractItemModel *transactions READ transactions CONSTANT)

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
    [[nodiscard]] const QString &totalIncomeText() const noexcept;
    [[nodiscard]] const QString &totalExpenseText() const noexcept;
    [[nodiscard]] const QString &balanceText() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] QAbstractItemModel *transactions() noexcept;

    Q_INVOKABLE void load();
    Q_INVOKABLE void showPreviousMonth();
    Q_INVOKABLE void showNextMonth();
    Q_INVOKABLE bool selectTimeZone(const QString &timeZoneId);
    Q_INVOKABLE bool selectFixedOffsetMinutes(int offsetMinutes);

signals:
    void selectedMonthChanged();
    void timeZoneChanged();
    void overviewChanged();
    void errorChanged();

private:
    void changeMonth(const QDate &month);
    bool applyTimeZone(const QTimeZone &timeZone);
    void setError(QString message);
    void clearError();

    GetMonthlyOverview &m_getMonthlyOverview;
    QTimeZone m_timeZone;
    QDate m_selectedMonth;
    TransactionListModel m_transactions;
    QString m_totalIncomeText{QStringLiteral("0.00")};
    QString m_totalExpenseText{QStringLiteral("0.00")};
    QString m_balanceText{QStringLiteral("0.00")};
    QString m_errorMessage;
};

} // namespace stukit::accounting::presentation
