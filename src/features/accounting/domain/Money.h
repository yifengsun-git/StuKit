#pragma once

#include <QtGlobal>

namespace stukit::accounting {

class Money final
{
public:
    static Money fromCents(qint64 cents) noexcept;

    [[nodiscard]] qint64 cents() const noexcept;
    [[nodiscard]] bool isZero() const noexcept;
    [[nodiscard]] bool isPositive() const noexcept;
    [[nodiscard]] bool isNegative() const noexcept;

    [[nodiscard]] Money operator+(const Money &other) const noexcept;
    [[nodiscard]] Money operator-(const Money &other) const noexcept;

    friend bool operator==(const Money &left, const Money &right) = default;

private:
    explicit Money(qint64 cents) noexcept;

    qint64 m_cents;
};

} // namespace stukit::accounting
