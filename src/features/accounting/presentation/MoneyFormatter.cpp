#include "MoneyFormatter.h"

#include <QtGlobal>

namespace stukit::accounting::presentation {

QString formatMoney(Money money)
{
    const qint64 cents = money.cents();
    const quint64 magnitude = cents < 0
        ? static_cast<quint64>(-(cents + 1)) + 1
        : static_cast<quint64>(cents);

    const QString sign = cents < 0 ? QStringLiteral("-") : QString{};
    const quint64 whole = magnitude / 100;
    const quint64 fraction = magnitude % 100;

    return QStringLiteral("%1%2.%3")
        .arg(sign)
        .arg(whole)
        .arg(fraction, 2, 10, QLatin1Char('0'));
}

} // namespace stukit::accounting::presentation
