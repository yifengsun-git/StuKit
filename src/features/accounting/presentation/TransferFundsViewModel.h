#pragma once

#include "AccountListModel.h"
#include "CategoryListModel.h"
#include "GetAccountingSetup.h"
#include "TransferFunds.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QTimeZone>

#include <vector>

namespace stukit::accounting::presentation {

class TransferFundsViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *availableAccounts READ availableAccounts CONSTANT)
    Q_PROPERTY(QAbstractItemModel *incomeCategories READ incomeCategories CONSTANT)
    Q_PROPERTY(QAbstractItemModel *expenseCategories READ expenseCategories CONSTANT)
    Q_PROPERTY(QString occurredDateText READ occurredDateText NOTIFY formChanged)
    Q_PROPERTY(QString occurredTimeText READ occurredTimeText NOTIFY formChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY optionsChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    TransferFundsViewModel(
        TransferFunds &transferFunds,
        GetAccountingSetup &getAccountingSetup,
        QTimeZone timeZone,
        QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *availableAccounts() noexcept;
    [[nodiscard]] QAbstractItemModel *incomeCategories() noexcept;
    [[nodiscard]] QAbstractItemModel *expenseCategories() noexcept;
    [[nodiscard]] const QString &occurredDateText() const noexcept;
    [[nodiscard]] const QString &occurredTimeText() const noexcept;
    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;

    void setTimeZone(QTimeZone timeZone);

    Q_INVOKABLE void prepare();
    Q_INVOKABLE bool submit(
        const QString &amountText,
        const QString &feeText,
        const QString &sourceAccountId,
        const QString &destinationAccountId,
        const QString &outgoingCategoryId,
        const QString &incomingCategoryId,
        const QString &feeCategoryId,
        const QString &dateText,
        const QString &timeText,
        const QString &note);

signals:
    void optionsChanged();
    void formChanged();
    void errorChanged();
    void transferCreated();

private:
    void setError(QString message);
    void clearError();

    TransferFunds &m_transferFunds;
    GetAccountingSetup &m_getAccountingSetup;
    QTimeZone m_timeZone;
    AccountListModel m_accounts;
    CategoryListModel m_incomeCategories;
    CategoryListModel m_expenseCategories;
    std::vector<Account> m_activeAccounts;
    std::vector<Category> m_activeIncomeCategories;
    std::vector<Category> m_activeExpenseCategories;
    QString m_occurredDateText;
    QString m_occurredTimeText;
    QString m_errorMessage;
};

} // namespace stukit::accounting::presentation
