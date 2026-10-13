#include "OpenMeteoUrlBuilder.h"

#include <QUrlQuery>

namespace stukit::weather {

QUrl OpenMeteoUrlBuilder::locationSearch(QString query)
{
    QUrl url(QStringLiteral(
        "https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("name"), query.trimmed());
    parameters.addQueryItem(QStringLiteral("count"), QStringLiteral("8"));
    parameters.addQueryItem(QStringLiteral("language"), QStringLiteral("zh"));
    parameters.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(parameters);
    return url;
}

QUrl OpenMeteoUrlBuilder::forecast(const WeatherLocation &location)
{
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery parameters;
    parameters.addQueryItem(
        QStringLiteral("latitude"),
        QString::number(location.latitude(), 'f', 6));
    parameters.addQueryItem(
        QStringLiteral("longitude"),
        QString::number(location.longitude(), 'f', 6));
    parameters.addQueryItem(
        QStringLiteral("current"),
        QStringLiteral("temperature_2m,apparent_temperature,"
                       "relative_humidity_2m,precipitation,weather_code,"
                       "wind_speed_10m,wind_direction_10m,is_day"));
    parameters.addQueryItem(
        QStringLiteral("daily"),
        QStringLiteral("weather_code,temperature_2m_max,temperature_2m_min,"
                       "precipitation_probability_max,precipitation_sum,"
                       "sunrise,sunset"));
    parameters.addQueryItem(QStringLiteral("timezone"), location.timezoneId());
    parameters.addQueryItem(QStringLiteral("forecast_days"), QStringLiteral("7"));
    parameters.addQueryItem(QStringLiteral("temperature_unit"),
                            QStringLiteral("celsius"));
    parameters.addQueryItem(QStringLiteral("wind_speed_unit"),
                            QStringLiteral("kmh"));
    parameters.addQueryItem(QStringLiteral("precipitation_unit"),
                            QStringLiteral("mm"));
    url.setQuery(parameters);
    return url;
}

} // namespace stukit::weather
