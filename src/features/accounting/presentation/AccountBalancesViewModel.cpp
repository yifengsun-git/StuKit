#include "AccountBalancesViewModel.h"

#include "MoneyFormatter.h"

#include <utility>
#include <variant>

namespace stukit::accounting::presentation {

AccountBalancesViewModel::AccountBalancesViewModel(
    GetAccountBalances &getAccountBalances,
    QObject *parent)
    : QObject(parent)
    , m_getAccountBalances(getAccountBalances)
    , m_accounts(this)
{
}

QAbstractItemModel *AccountBalancesViewModel::accounts() noexcept
{
    return &m_accounts;
}

const QString &AccountBalancesViewModel::activeTotalText() const noexcept
{
    return m_activeTotalText;
}

bool AccountBalancesViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &AccountBalancesViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

void AccountBalancesViewModel::load()
{
    auto result = m_getAccountBalances.execute();
    if (std::holds_alternative<GetAccountBalancesError>(result)) {
        m_accounts.clear();
        m_activeTotalText = QStringLiteral("0.00");
        const auto error = std::get<GetAccountBalancesError>(result);
        setError(error == GetAccountBalancesError::ArithmeticOverflow
                     ? QStringLiteral("账户余额超出支持范围")
                     : QStringLiteral("无法读取账户余额"));
        emit balancesChanged();
        return;
    }

    auto balances = std::get<AccountBalances>(std::move(result));
    m_activeTotalText = formatMoney(balances.activeTotal);
    m_accounts.replaceBalances(std::move(balances.accounts));
    clearError();
    emit balancesChanged();
}

void AccountBalancesViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void AccountBalancesViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::accounting::presentation
