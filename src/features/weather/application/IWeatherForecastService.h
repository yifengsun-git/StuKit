#pragma once

#include "WeatherServiceTypes.h"

namespace stukit::weather {

class IWeatherForecastService
{
public:
    virtual ~IWeatherForecastService() = default;

    // Implementations must invoke completion asynchronously at most once.
    virtual WeatherRequestId fetch(
        WeatherLocation location,
        ForecastCompletion completion) = 0;
    virtual void cancel(WeatherRequestId requestId) = 0;
};

} // namespace stukit::weather
