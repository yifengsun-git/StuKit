#pragma once

#include "IWeatherForecastCache.h"

#include <QString>

namespace stukit::weather {

class JsonWeatherForecastCache final : public IWeatherForecastCache
{
public:
    explicit JsonWeatherForecastCache(QString filePath);

    [[nodiscard]] std::optional<WeatherForecast> load(
        const WeatherLocation &location) override;
    [[nodiscard]] bool save(const WeatherForecast &forecast) override;

private:
    QString m_filePath;
};

} // namespace stukit::weather
