#include "OpenMeteoJsonParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#include <cmath>
#include <optional>

namespace stukit::weather {
namespace {

std::optional<QJsonObject> objectFrom(const QByteArray &json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }
    return document.object();
}

std::optional<double> number(const QJsonObject &object, const char *key)
{
    const QJsonValue value = object.value(QLatin1StringView(key));
    return value.isDouble()
        ? std::optional<double>{value.toDouble()}
        : std::nullopt;
}

std::optional<int> integer(const QJsonObject &object, const char *key)
{
    const auto value = number(object, key);
    if (!value.has_value() || std::floor(*value) != *value) {
        return std::nullopt;
    }
    return static_cast<int>(*value);
}

std::optional<QDateTime> localDateTime(
    const QString &text,
    const QTimeZone &zone)
{
    const QStringList parts = text.split(QLatin1Char('T'));
    if (parts.size() != 2) {
        return std::nullopt;
    }
    const QDate date = QDate::fromString(parts[0], Qt::ISODate);
    const QTime time = QTime::fromString(parts[1], QStringLiteral("HH:mm"));
    if (!date.isValid() || !time.isValid()) {
        return std::nullopt;
    }
    return QDateTime(date, time, zone);
}

bool sameSize(const QJsonArray &reference, const QJsonArray &candidate)
{
    return candidate.size() == reference.size();
}

} // namespace

LocationSearchResult OpenMeteoJsonParser::parseLocations(
    const QByteArray &json)
{
    const auto root = objectFrom(json);
    if (!root.has_value()) {
        return WeatherServiceError::InvalidResponse;
    }

    const QJsonValue resultsValue = root->value(QStringLiteral("results"));
    if (resultsValue.isUndefined()) {
        return std::vector<WeatherLocation>{};
    }
    if (!resultsValue.isArray()) {
        return WeatherServiceError::InvalidResponse;
    }

    std::vector<WeatherLocation> locations;
    for (const QJsonValue &value : resultsValue.toArray()) {
        if (!value.isObject()) {
            return WeatherServiceError::InvalidResponse;
        }
        const QJsonObject item = value.toObject();
        const auto latitude = number(item, "latitude");
        const auto longitude = number(item, "longitude");
        if (!latitude.has_value() || !longitude.has_value()) {
            return WeatherServiceError::InvalidResponse;
        }
        auto location = WeatherLocation::create(WeatherLocationData{
            .name = item.value(QStringLiteral("name")).toString(),
            .country = item.value(QStringLiteral("country")).toString(),
            .administrativeArea =
                item.value(QStringLiteral("admin1")).toString(),
            .timezoneId = item.value(QStringLiteral("timezone")).toString(),
            .latitude = *latitude,
            .longitude = *longitude,
        });
        if (!location.has_value()) {
            return WeatherServiceError::InvalidResponse;
        }
        locations.push_back(std::move(*location));
    }
    return locations;
}

ForecastResult OpenMeteoJsonParser::parseForecast(
    const QByteArray &json,
    const WeatherLocation &location,
    QDateTime fetchedAtUtc)
{
    const auto root = objectFrom(json);
    if (!root.has_value()
        || !root->value(QStringLiteral("current")).isObject()
        || !root->value(QStringLiteral("daily")).isObject()) {
        return WeatherServiceError::InvalidResponse;
    }

    const QTimeZone zone(location.timezoneId().toUtf8());
    const QJsonObject currentObject =
        root->value(QStringLiteral("current")).toObject();
    const auto observedAt = localDateTime(
        currentObject.value(QStringLiteral("time")).toString(), zone);
    const auto temperature = number(currentObject, "temperature_2m");
    const auto apparent = number(currentObject, "apparent_temperature");
    const auto humidity = integer(currentObject, "relative_humidity_2m");
    const auto precipitation = number(currentObject, "precipitation");
    const auto weatherCode = integer(currentObject, "weather_code");
    const auto windSpeed = number(currentObject, "wind_speed_10m");
    const auto windDirection = integer(currentObject, "wind_direction_10m");
    const auto isDay = integer(currentObject, "is_day");
    if (!observedAt || !temperature || !apparent || !humidity
        || !precipitation || !weatherCode || !windSpeed || !windDirection
        || !isDay || (*isDay != 0 && *isDay != 1)) {
        return WeatherServiceError::InvalidResponse;
    }

    const QJsonObject dailyObject =
        root->value(QStringLiteral("daily")).toObject();
    const QJsonArray dates = dailyObject.value(QStringLiteral("time")).toArray();
    const QJsonArray codes =
        dailyObject.value(QStringLiteral("weather_code")).toArray();
    const QJsonArray maximums =
        dailyObject.value(QStringLiteral("temperature_2m_max")).toArray();
    const QJsonArray minimums =
        dailyObject.value(QStringLiteral("temperature_2m_min")).toArray();
    const QJsonArray probabilities = dailyObject.value(
        QStringLiteral("precipitation_probability_max")).toArray();
    const QJsonArray precipitationSums =
        dailyObject.value(QStringLiteral("precipitation_sum")).toArray();
    const QJsonArray sunrises =
        dailyObject.value(QStringLiteral("sunrise")).toArray();
    const QJsonArray sunsets =
        dailyObject.value(QStringLiteral("sunset")).toArray();
    if (dates.isEmpty()
        || !sameSize(dates, codes)
        || !sameSize(dates, maximums)
        || !sameSize(dates, minimums)
        || !sameSize(dates, probabilities)
        || !sameSize(dates, precipitationSums)
        || !sameSize(dates, sunrises)
        || !sameSize(dates, sunsets)) {
        return WeatherServiceError::InvalidResponse;
    }

    std::vector<DailyWeather> daily;
    daily.reserve(static_cast<std::size_t>(dates.size()));
    for (qsizetype index = 0; index < dates.size(); ++index) {
        const QDate date = QDate::fromString(dates[index].toString(), Qt::ISODate);
        const auto sunrise = localDateTime(sunrises[index].toString(), zone);
        const auto sunset = localDateTime(sunsets[index].toString(), zone);
        if (!date.isValid() || !codes[index].isDouble()
            || !maximums[index].isDouble() || !minimums[index].isDouble()
            || !probabilities[index].isDouble()
            || !precipitationSums[index].isDouble()
            || !sunrise || !sunset) {
            return WeatherServiceError::InvalidResponse;
        }
        daily.push_back(DailyWeather{
            .date = date,
            .minimumTemperatureCelsius = minimums[index].toDouble(),
            .maximumTemperatureCelsius = maximums[index].toDouble(),
            .precipitationProbabilityPercent = probabilities[index].toInt(),
            .precipitationMillimetres = precipitationSums[index].toDouble(),
            .condition = conditionFromWmoCode(codes[index].toInt()),
            .sunrise = *sunrise,
            .sunset = *sunset,
        });
    }

    auto forecast = WeatherForecast::create(WeatherForecastData{
        .location = location,
        .current = CurrentWeather{
            .observedAt = *observedAt,
            .temperatureCelsius = *temperature,
            .apparentTemperatureCelsius = *apparent,
            .relativeHumidityPercent = *humidity,
            .precipitationMillimetres = *precipitation,
            .windSpeedKilometresPerHour = *windSpeed,
            .windDirectionDegrees = *windDirection,
            .condition = conditionFromWmoCode(*weatherCode),
            .daytime = *isDay == 1,
        },
        .daily = std::move(daily),
        .fetchedAtUtc = fetchedAtUtc.toUTC(),
    });
    return forecast.has_value()
        ? ForecastResult{std::move(*forecast)}
        : ForecastResult{WeatherServiceError::InvalidResponse};
}

} // namespace stukit::weather
