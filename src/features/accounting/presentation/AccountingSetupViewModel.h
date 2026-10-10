#pragma once

#include "AccountListModel.h"
#include "CategoryListModel.h"
#include "CreateAccount.h"
#include "CreateCategory.h"
#include "GetAccountingSetup.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

namespace stukit::accounting::presentation {

class AccountingSetupViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *accounts READ accounts CONSTANT)
    Q_PROPERTY(QAbstractItemModel *categories READ categories CONSTANT)
    Q_PROPERTY(bool hasAccounts READ hasAccounts NOTIFY setupChanged)
    Q_PROPERTY(bool hasIncomeCategories READ hasIncomeCategories NOTIFY setupChanged)
    Q_PROPERTY(bool hasExpenseCategories READ hasExpenseCategories NOTIFY setupChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    AccountingSetupViewModel(
        GetAccountingSetup &getAccountingSetup,
        CreateAccount &createAccount,
        CreateCategory &createCategory,
        QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *accounts() noexcept;
    [[nodiscard]] QAbstractItemModel *categories() noexcept;
    [[nodiscard]] bool hasAccounts() const noexcept;
    [[nodiscard]] bool hasIncomeCategories() const noexcept;
    [[nodiscard]] bool hasExpenseCategories() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;

    Q_INVOKABLE void load();
    Q_INVOKABLE bool createAccount(
        const QString &name, const QString &openingBalanceText);
    Q_INVOKABLE bool createCategory(const QString &name, bool isIncome);

signals:
    void setupChanged();
    void errorChanged();

private:
    void setError(QString message);
    void clearError();

    GetAccountingSetup &m_getAccountingSetup;
    CreateAccount &m_createAccount;
    CreateCategory &m_createCategory;
    AccountListModel m_accounts;
    CategoryListModel m_categories;
    bool m_hasAccounts{false};
    bool m_hasIncomeCategories{false};
    bool m_hasExpenseCategories{false};
    QString m_errorMessage;
};

} // namespace stukit::accounting::presentation
