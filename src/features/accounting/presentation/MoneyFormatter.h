#pragma once

#include "Money.h"

#include <QString>

namespace stukit::accounting::presentation {

[[nodiscard]] QString formatMoney(Money money);

} // namespace stukit::accounting::presentation
