#pragma once

#include "WeatherForecast.h"
#include "WeatherLocation.h"

#include <QtGlobal>

#include <functional>
#include <variant>
#include <vector>

namespace stukit::weather {

using WeatherRequestId = quint64;
inline constexpr WeatherRequestId InvalidWeatherRequestId = 0;

enum class WeatherServiceError
{
    InvalidInput,
    ConfigurationMissing,
    AuthenticationFailed,
    NetworkUnavailable,
    RateLimited,
    ServiceUnavailable,
    InvalidResponse,
    Cancelled,
};

using LocationSearchResult =
    std::variant<std::vector<WeatherLocation>, WeatherServiceError>;
using ForecastResult = std::variant<WeatherForecast, WeatherServiceError>;
using LocationSearchCompletion = std::function<void(LocationSearchResult)>;
using ForecastCompletion = std::function<void(ForecastResult)>;

} // namespace stukit::weather
