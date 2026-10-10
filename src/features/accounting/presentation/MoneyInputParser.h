#pragma once

#include "Money.h"

#include <QString>

#include <optional>

namespace stukit::accounting::presentation {

[[nodiscard]] std::optional<Money> parseMoneyInput(const QString &text);

} // namespace stukit::accounting::presentation
