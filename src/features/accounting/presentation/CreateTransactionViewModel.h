#pragma once

#include "AccountListModel.h"
#include "CategoryListModel.h"
#include "CreateTransaction.h"
#include "DeleteTransaction.h"
#include "GetAccountingSetup.h"
#include "GetTransaction.h"
#include "UpdateTransaction.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QTimeZone>

#include <vector>

namespace stukit::accounting::presentation {

class CreateTransactionViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *availableAccounts READ availableAccounts CONSTANT)
    Q_PROPERTY(QAbstractItemModel *availableCategories READ availableCategories CONSTANT)
    Q_PROPERTY(bool income READ isIncome WRITE setIncome NOTIFY transactionTypeChanged)
    Q_PROPERTY(QString amountText READ amountText WRITE setAmountText NOTIFY formChanged)
    Q_PROPERTY(QString note READ note WRITE setNote NOTIFY formChanged)
    Q_PROPERTY(QString selectedAccountId READ selectedAccountId WRITE setSelectedAccountId NOTIFY formChanged)
    Q_PROPERTY(QString selectedCategoryId READ selectedCategoryId WRITE setSelectedCategoryId NOTIFY formChanged)
    Q_PROPERTY(QString occurredDateText READ occurredDateText WRITE setOccurredDateText NOTIFY formChanged)
    Q_PROPERTY(QString occurredTimeText READ occurredTimeText WRITE setOccurredTimeText NOTIFY formChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY optionsChanged)
    Q_PROPERTY(bool canSubmit READ canSubmit NOTIFY formChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(bool editing READ isEditing NOTIFY editingChanged)
    Q_PROPERTY(QString editingTransactionId READ editingTransactionId NOTIFY editingChanged)
    Q_PROPERTY(QString submitButtonText READ submitButtonText NOTIFY editingChanged)

public:
    CreateTransactionViewModel(
        CreateTransaction &createTransaction,
        UpdateTransaction &updateTransaction,
        DeleteTransaction &deleteTransaction,
        GetTransaction &getTransaction,
        GetAccountingSetup &getAccountingSetup,
        QTimeZone timeZone,
        QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *availableAccounts() noexcept;
    [[nodiscard]] QAbstractItemModel *availableCategories() noexcept;
    [[nodiscard]] bool isIncome() const noexcept;
    [[nodiscard]] const QString &amountText() const noexcept;
    [[nodiscard]] const QString &note() const noexcept;
    [[nodiscard]] const QString &selectedAccountId() const noexcept;
    [[nodiscard]] const QString &selectedCategoryId() const noexcept;
    [[nodiscard]] const QString &occurredDateText() const noexcept;
    [[nodiscard]] const QString &occurredTimeText() const noexcept;
    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] bool canSubmit() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] bool isEditing() const noexcept;
    [[nodiscard]] QString editingTransactionId() const;
    [[nodiscard]] QString submitButtonText() const;
    [[nodiscard]] const QTimeZone &timeZone() const noexcept;

    void setIncome(bool income);
    void setAmountText(QString amountText);
    void setNote(QString note);
    void setSelectedAccountId(QString accountId);
    void setSelectedCategoryId(QString categoryId);
    void setOccurredDateText(QString dateText);
    void setOccurredTimeText(QString timeText);
    void setTimeZone(QTimeZone timeZone);

    Q_INVOKABLE void loadSetup();
    Q_INVOKABLE void resetForNewTransaction();
    Q_INVOKABLE bool beginEditing(const QString &transactionId);
    Q_INVOKABLE bool deleteTransaction(const QString &transactionId);
    Q_INVOKABLE bool submit();

signals:
    void transactionTypeChanged();
    void optionsChanged();
    void formChanged();
    void errorChanged();
    void editingChanged();
    void transactionCreated();
    void transactionUpdated();
    void transactionDeleted();

private:
    void filterCategories();
    void setError(QString message);
    void clearError();
    [[nodiscard]] bool containsAccountId(const QUuid &id) const;
    [[nodiscard]] bool containsCategoryId(const QUuid &id) const;

    CreateTransaction &m_createTransaction;
    UpdateTransaction &m_updateTransaction;
    DeleteTransaction &m_deleteTransaction;
    GetTransaction &m_getTransaction;
    GetAccountingSetup &m_getAccountingSetup;
    QTimeZone m_timeZone;
    AccountListModel m_accounts;
    CategoryListModel m_categories;
    std::vector<Account> m_activeAccounts;
    std::vector<Category> m_activeCategories;
    std::vector<Category> m_filteredCategories;
    bool m_income{false};
    QString m_amountText;
    QString m_note;
    QString m_selectedAccountId;
    QString m_selectedCategoryId;
    QString m_occurredDateText;
    QString m_occurredTimeText;
    QString m_errorMessage;
    QUuid m_editingTransactionId;
};

} // namespace stukit::accounting::presentation
