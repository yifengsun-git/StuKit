#pragma once

#include "WeatherLocation.h"

#include <QString>
#include <QUrl>

namespace stukit::weather {

class QWeatherUrlBuilder final
{
public:
    [[nodiscard]] static QUrl locationSearch(
        const QString &apiHost,
        QString query);
    [[nodiscard]] static QUrl currentWeather(
        const QString &apiHost,
        const WeatherLocation &location);
    [[nodiscard]] static QUrl dailyForecast(
        const QString &apiHost,
        const WeatherLocation &location);
    [[nodiscard]] static bool isValidApiHost(const QString &apiHost);
};

} // namespace stukit::weather
