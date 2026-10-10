#include "MoneyInputParser.h"

#include <limits>

namespace stukit::accounting::presentation {

std::optional<Money> parseMoneyInput(const QString &text)
{
    const QString value = text.trimmed();
    if (value.isEmpty()) {
        return std::nullopt;
    }

    qsizetype position = 0;
    bool negative = false;
    if (value.front() == QLatin1Char('-') || value.front() == QLatin1Char('+')) {
        negative = value.front() == QLatin1Char('-');
        ++position;
    }
    if (position == value.size()) {
        return std::nullopt;
    }

    QString digits;
    qsizetype fractionDigits = -1;
    for (; position < value.size(); ++position) {
        const QChar character = value.at(position);
        if (character == QLatin1Char('.')) {
            if (fractionDigits >= 0 || digits.isEmpty()) {
                return std::nullopt;
            }
            fractionDigits = 0;
            continue;
        }
        if (!character.isDigit() || character.unicode() > QLatin1Char('9').unicode()) {
            return std::nullopt;
        }
        if (fractionDigits >= 0 && ++fractionDigits > 2) {
            return std::nullopt;
        }
        digits.append(character);
    }

    if (fractionDigits == 0) {
        return std::nullopt;
    }
    if (fractionDigits < 0) {
        digits.append(QStringLiteral("00"));
    } else if (fractionDigits == 1) {
        digits.append(QLatin1Char('0'));
    }

    const quint64 positiveLimit = static_cast<quint64>(
        std::numeric_limits<qint64>::max());
    const quint64 limit = negative ? positiveLimit + 1 : positiveLimit;
    quint64 magnitude = 0;
    for (const QChar character : digits) {
        const quint64 digit = static_cast<quint64>(character.unicode()
                                                  - QLatin1Char('0').unicode());
        if (magnitude > (limit - digit) / 10) {
            return std::nullopt;
        }
        magnitude = magnitude * 10 + digit;
    }

    if (negative && magnitude == positiveLimit + 1) {
        return Money::fromCents(std::numeric_limits<qint64>::min());
    }

    const qint64 cents = static_cast<qint64>(magnitude);
    return Money::fromCents(negative ? -cents : cents);
}

} // namespace stukit::accounting::presentation
