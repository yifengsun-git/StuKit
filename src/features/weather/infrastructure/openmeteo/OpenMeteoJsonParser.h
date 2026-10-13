#pragma once

#include "WeatherServiceTypes.h"

#include <QByteArray>

namespace stukit::weather {

class OpenMeteoJsonParser final
{
public:
    [[nodiscard]] static LocationSearchResult parseLocations(
        const QByteArray &json);
    [[nodiscard]] static ForecastResult parseForecast(
        const QByteArray &json,
        const WeatherLocation &location,
        QDateTime fetchedAtUtc);
};

} // namespace stukit::weather
