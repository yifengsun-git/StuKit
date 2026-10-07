#include "Money.h"

namespace stukit::accounting {

Money Money::fromCents(qint64 cents) noexcept
{
    return Money(cents);
}

qint64 Money::cents() const noexcept
{
    return m_cents;
}

bool Money::isZero() const noexcept
{
    return m_cents == 0;
}

bool Money::isPositive() const noexcept
{
    return m_cents > 0;
}

bool Money::isNegative() const noexcept
{
    return m_cents < 0;
}

Money Money::operator+(const Money &other) const noexcept
{
    return Money(m_cents + other.m_cents);
}

Money Money::operator-(const Money &other) const noexcept
{
    return Money(m_cents - other.m_cents);
}

Money::Money(qint64 cents) noexcept
    : m_cents(cents)
{
}

} // namespace stukit::accounting
