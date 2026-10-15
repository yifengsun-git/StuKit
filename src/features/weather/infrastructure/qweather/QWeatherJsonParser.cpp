#include "QWeatherJsonParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

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

std::optional<double> numberValue(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    if (value.isString()) {
        bool valid = false;
        const double number = value.toString().toDouble(&valid);
        if (valid) {
            return number;
        }
    }
    return std::nullopt;
}

std::optional<double> number(const QJsonObject &object, const char *key)
{
    return numberValue(object.value(QLatin1StringView(key)));
}

std::optional<int> integerValue(const QJsonValue &value)
{
    const auto number = numberValue(value);
    if (!number.has_value() || std::floor(*number) != *number) {
        return std::nullopt;
    }
    return static_cast<int>(*number);
}

std::optional<QJsonObject> childObject(
    const QJsonObject &object,
    const char *key)
{
    const QJsonValue value = object.value(QLatin1StringView(key));
    return value.isObject()
        ? std::optional<QJsonObject>{value.toObject()}
        : std::nullopt;
}

std::optional<double> measurement(
    const QJsonObject &object,
    const char *key)
{
    const auto valueObject = childObject(object, key);
    return valueObject.has_value()
        ? number(*valueObject, "value")
        : std::nullopt;
}

std::optional<QDateTime> dateTimeInZone(
    const QString &text,
    const QTimeZone &zone)
{
    QDateTime result = QDateTime::fromString(text, Qt::ISODate);
    if (!result.isValid()) {
        return std::nullopt;
    }
    if (result.timeSpec() == Qt::LocalTime) {
        result.setTimeZone(zone);
    } else {
        result = result.toTimeZone(zone);
    }
    return result;
}

WeatherCondition conditionFromQWeatherCode(int code) noexcept
{
    switch (code) {
    case 100: return WeatherCondition::Clear;
    case 101:
    case 102: return WeatherCondition::MainlyClear;
    case 103: return WeatherCondition::PartlyCloudy;
    case 104: return WeatherCondition::Overcast;
    case 300:
    case 301: return WeatherCondition::RainShowers;
    case 302:
    case 303:
    case 304: return WeatherCondition::Thunderstorm;
    case 305:
    case 306:
    case 307:
    case 308:
    case 309:
    case 310:
    case 311:
    case 312:
    case 313:
    case 314:
    case 315:
    case 316:
    case 317:
    case 318:
    case 350:
    case 351:
    case 399: return WeatherCondition::Rain;
    case 400:
    case 401:
    case 402:
    case 403:
    case 408:
    case 409:
    case 410:
    case 456:
    case 457:
    case 499: return WeatherCondition::Snow;
    case 404:
    case 405:
    case 406:
    case 407: return WeatherCondition::SnowShowers;
    case 500:
    case 501:
    case 502:
    case 503:
    case 504:
    case 507:
    case 508:
    case 509:
    case 510:
    case 511:
    case 512:
    case 513:
    case 514:
    case 515: return WeatherCondition::Fog;
    default: return WeatherCondition::Unknown;
    }
}

QString administrativeArea(const QJsonObject &item)
{
    const QString city = item.value(QStringLiteral("adm2")).toString().trimmed();
    const QString province = item.value(QStringLiteral("adm1")).toString().trimmed();
    if (city.isEmpty() || city == province) {
        return province;
    }
    return province.isEmpty()
        ? city
        : QStringLiteral("%1 · %2").arg(province, city);
}

} // namespace

LocationSearchResult QWeatherJsonParser::parseLocations(const QByteArray &json)
{
    const auto root = objectFrom(json);
    if (!root.has_value()) {
        return WeatherServiceError::InvalidResponse;
    }
    const QString responseCode = root->value(QStringLiteral("code")).toString();
    if (responseCode == QStringLiteral("404")) {
        return std::vector<WeatherLocation>{};
    }
    if (responseCode == QStringLiteral("401")
        || responseCode == QStringLiteral("403")) {
        return WeatherServiceError::AuthenticationFailed;
    }
    if (responseCode == QStringLiteral("429")) {
        return WeatherServiceError::RateLimited;
    }
    if (responseCode.startsWith(QLatin1Char('5'))) {
        return WeatherServiceError::ServiceUnavailable;
    }
    if (responseCode != QStringLiteral("200")) {
        return WeatherServiceError::InvalidResponse;
    }

    const QJsonValue locationsValue = root->value(QStringLiteral("location"));
    if (locationsValue.isUndefined() || locationsValue.isNull()) {
        return std::vector<WeatherLocation>{};
    }
    if (!locationsValue.isArray()) {
        return WeatherServiceError::InvalidResponse;
    }

    std::vector<WeatherLocation> locations;
    for (const QJsonValue &value : locationsValue.toArray()) {
        if (!value.isObject()) {
            return WeatherServiceError::InvalidResponse;
        }
        const QJsonObject item = value.toObject();
        const auto latitude = number(item, "lat");
        const auto longitude = number(item, "lon");
        if (!latitude.has_value() || !longitude.has_value()) {
            return WeatherServiceError::InvalidResponse;
        }

        auto location = WeatherLocation::create(WeatherLocationData{
            .name = item.value(QStringLiteral("name")).toString(),
            .country = item.value(QStringLiteral("country")).toString(),
            .administrativeArea = administrativeArea(item),
            .timezoneId = item.value(QStringLiteral("tz")).toString(),
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

ForecastResult QWeatherJsonParser::parseForecast(
    const QByteArray &currentJson,
    const QByteArray &dailyJson,
    const WeatherLocation &location,
    QDateTime fetchedAtUtc)
{
    const auto currentRoot = objectFrom(currentJson);
    const auto dailyRoot = objectFrom(dailyJson);
    if (!currentRoot.has_value() || !dailyRoot.has_value()) {
        return WeatherServiceError::InvalidResponse;
    }

    const auto condition = childObject(*currentRoot, "condition");
    const auto wind = childObject(*currentRoot, "wind");
    const auto precipitation = childObject(*currentRoot, "precipitation");
    if (!condition || !wind || !precipitation) {
        return WeatherServiceError::InvalidResponse;
    }
    const auto direction = childObject(*wind, "direction");
    const auto speed = childObject(*wind, "speed");
    const auto amount = childObject(*precipitation, "amount");
    const auto temperature = measurement(*currentRoot, "temperature");
    const auto apparent = measurement(*currentRoot, "feelsLike");
    const auto humidity = number(*currentRoot, "humidity");
    const auto windSpeed = speed ? number(*speed, "value") : std::nullopt;
    const auto precipitationAmount = amount
        ? number(*amount, "value") : std::nullopt;
    const auto windDirection = direction
        ? integerValue(direction->value(QStringLiteral("degree")))
        : std::nullopt;
    const auto currentCode = integerValue(
        condition->value(QStringLiteral("code")));
    if (!temperature || !apparent || !humidity || !windSpeed
        || !precipitationAmount || !windDirection || !currentCode) {
        return WeatherServiceError::InvalidResponse;
    }
    if (*humidity < 0.0 || *humidity > 1.0) {
        return WeatherServiceError::InvalidResponse;
    }
    double windSpeedKilometresPerHour = *windSpeed;
    const QString windSpeedUnit = speed->value(QStringLiteral("unit")).toString();
    if (windSpeedUnit == QStringLiteral("m/s")) {
        windSpeedKilometresPerHour *= 3.6;
    } else if (windSpeedUnit != QStringLiteral("km/h")) {
        return WeatherServiceError::InvalidResponse;
    }

    const QJsonValue daysValue = dailyRoot->value(QStringLiteral("days"));
    if (!daysValue.isArray() || daysValue.toArray().isEmpty()) {
        return WeatherServiceError::InvalidResponse;
    }

    const QTimeZone zone(location.timezoneId().toUtf8());
    std::vector<DailyWeather> daily;
    daily.reserve(static_cast<std::size_t>(daysValue.toArray().size()));
    for (const QJsonValue &value : daysValue.toArray()) {
        if (!value.isObject()) {
            return WeatherServiceError::InvalidResponse;
        }
        const QJsonObject item = value.toObject();
        const auto forecastStart = dateTimeInZone(
            item.value(QStringLiteral("forecastStartTime")).toString(), zone);
        const auto astro = childObject(item, "astro");
        const auto daytime = childObject(item, "daytime");
        const auto nighttime = childObject(item, "nighttime");
        if (!forecastStart || !astro || !daytime || !nighttime) {
            return WeatherServiceError::InvalidResponse;
        }
        const auto sunrise = dateTimeInZone(
            astro->value(QStringLiteral("sunrise")).toString(), zone);
        const auto sunset = dateTimeInZone(
            astro->value(QStringLiteral("sunset")).toString(), zone);
        const auto minimum = measurement(item, "temperatureMin");
        const auto maximum = measurement(item, "temperatureMax");
        const auto dayCondition = childObject(*daytime, "condition");
        const auto dayPrecipitation = childObject(*daytime, "precipitation");
        const auto nightPrecipitation = childObject(*nighttime, "precipitation");
        if (!sunrise || !sunset || !minimum || !maximum
            || !dayCondition || !dayPrecipitation || !nightPrecipitation) {
            return WeatherServiceError::InvalidResponse;
        }
        const auto dayCode = integerValue(
            dayCondition->value(QStringLiteral("code")));
        const auto dayProbability = number(*dayPrecipitation, "probability");
        const auto nightProbability = number(*nightPrecipitation, "probability");
        const auto dayAmount = childObject(*dayPrecipitation, "amount");
        const auto nightAmount = childObject(*nightPrecipitation, "amount");
        const auto dayPrecipitationMillimetres = dayAmount
            ? number(*dayAmount, "value") : std::nullopt;
        const auto nightPrecipitationMillimetres = nightAmount
            ? number(*nightAmount, "value") : std::nullopt;
        if (!dayCode || !dayProbability || !nightProbability
            || !dayPrecipitationMillimetres
            || !nightPrecipitationMillimetres
            || *dayProbability < 0.0 || *dayProbability > 1.0
            || *nightProbability < 0.0 || *nightProbability > 1.0) {
            return WeatherServiceError::InvalidResponse;
        }

        daily.push_back(DailyWeather{
            .date = forecastStart->date(),
            .minimumTemperatureCelsius = *minimum,
            .maximumTemperatureCelsius = *maximum,
            .precipitationProbabilityPercent =
                qRound(std::max(*dayProbability, *nightProbability) * 100.0),
            .precipitationMillimetres = *dayPrecipitationMillimetres
                + *nightPrecipitationMillimetres,
            .condition = conditionFromQWeatherCode(*dayCode),
            .sunrise = *sunrise,
            .sunset = *sunset,
        });
    }

    const QDateTime observedAt = fetchedAtUtc.toTimeZone(zone);
    bool daytimeNow = observedAt.time().hour() >= 6
        && observedAt.time().hour() < 18;
    for (const DailyWeather &day : daily) {
        if (day.date == observedAt.date()) {
            daytimeNow = observedAt >= day.sunrise && observedAt < day.sunset;
            break;
        }
    }

    auto forecast = WeatherForecast::create(WeatherForecastData{
        .location = location,
        .current = CurrentWeather{
            .observedAt = observedAt,
            .temperatureCelsius = *temperature,
            .apparentTemperatureCelsius = *apparent,
            .relativeHumidityPercent =
                qRound(*humidity * 100.0),
            .precipitationMillimetres = *precipitationAmount,
            .windSpeedKilometresPerHour = windSpeedKilometresPerHour,
            .windDirectionDegrees = *windDirection,
            .condition = conditionFromQWeatherCode(*currentCode),
            .daytime = daytimeNow,
        },
        .daily = std::move(daily),
        .fetchedAtUtc = fetchedAtUtc.toUTC(),
    });
    return forecast.has_value()
        ? ForecastResult{std::move(*forecast)}
        : ForecastResult{WeatherServiceError::InvalidResponse};
}

} // namespace stukit::weather
