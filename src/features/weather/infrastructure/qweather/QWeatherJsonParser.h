#pragma once

#include "WeatherServiceTypes.h"

#include <QByteArray>

namespace stukit::weather {

class QWeatherJsonParser final
{
public:
    [[nodiscard]] static LocationSearchResult parseLocations(
        const QByteArray &json);
    [[nodiscard]] static ForecastResult parseForecast(
        const QByteArray &currentJson,
        const QByteArray &dailyJson,
        const WeatherLocation &location,
        QDateTime fetchedAtUtc);
};

} // namespace stukit::weather
