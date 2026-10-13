#pragma once

#include "WeatherForecast.h"
#include "WeatherLocation.h"

#include <optional>

namespace stukit::weather {

class IWeatherForecastCache
{
public:
    virtual ~IWeatherForecastCache() = default;

    [[nodiscard]] virtual std::optional<WeatherForecast> load(
        const WeatherLocation &location) = 0;
    [[nodiscard]] virtual bool save(const WeatherForecast &forecast) = 0;
};

} // namespace stukit::weather
