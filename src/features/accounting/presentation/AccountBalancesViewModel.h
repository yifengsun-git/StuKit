#pragma once

#include "AccountBalanceListModel.h"
#include "GetAccountBalances.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

namespace stukit::accounting::presentation {

class AccountBalancesViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *accounts READ accounts CONSTANT)
    Q_PROPERTY(QString activeTotalText READ activeTotalText NOTIFY balancesChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    explicit AccountBalancesViewModel(
        GetAccountBalances &getAccountBalances,
        QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *accounts() noexcept;
    [[nodiscard]] const QString &activeTotalText() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;

    Q_INVOKABLE void load();

signals:
    void balancesChanged();
    void errorChanged();

private:
    void setError(QString message);
    void clearError();

    GetAccountBalances &m_getAccountBalances;
    AccountBalanceListModel m_accounts;
    QString m_activeTotalText{QStringLiteral("0.00")};
    QString m_errorMessage;
};

} // namespace stukit::accounting::presentation
