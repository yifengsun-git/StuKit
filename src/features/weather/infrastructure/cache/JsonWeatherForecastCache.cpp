#include "JsonWeatherForecastCache.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <cmath>
#include <limits>
#include <utility>

namespace stukit::weather {
namespace {

constexpr int CacheSchemaVersion = 1;

QJsonObject locationToJson(const WeatherLocation &location)
{
    return {
        {QStringLiteral("name"), location.name()},
        {QStringLiteral("country"), location.country()},
        {QStringLiteral("administrativeArea"), location.administrativeArea()},
        {QStringLiteral("timezoneId"), location.timezoneId()},
        {QStringLiteral("latitude"), location.latitude()},
        {QStringLiteral("longitude"), location.longitude()},
    };
}

std::optional<WeatherLocation> locationFromJson(const QJsonObject &object)
{
    return WeatherLocation::create(WeatherLocationData{
        .name = object.value(QStringLiteral("name")).toString(),
        .country = object.value(QStringLiteral("country")).toString(),
        .administrativeArea = object.value(
            QStringLiteral("administrativeArea")).toString(),
        .timezoneId = object.value(QStringLiteral("timezoneId")).toString(),
        .latitude = object.value(QStringLiteral("latitude")).toDouble(
            std::numeric_limits<double>::quiet_NaN()),
        .longitude = object.value(QStringLiteral("longitude")).toDouble(
            std::numeric_limits<double>::quiet_NaN()),
    });
}

bool sameLocation(const WeatherLocation &left, const WeatherLocation &right)
{
    return left.name() == right.name()
        && left.timezoneId() == right.timezoneId()
        && std::abs(left.latitude() - right.latitude()) < 0.000001
        && std::abs(left.longitude() - right.longitude()) < 0.000001;
}

std::optional<WeatherCondition> conditionFromJson(const QJsonValue &value)
{
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const int raw = value.toInt(-1);
    if (raw < static_cast<int>(WeatherCondition::Clear)
        || raw > static_cast<int>(WeatherCondition::Unknown)) {
        return std::nullopt;
    }
    return static_cast<WeatherCondition>(raw);
}

QJsonObject currentToJson(const CurrentWeather &current)
{
    return {
        {QStringLiteral("observedAt"), current.observedAt.toString(Qt::ISODateWithMs)},
        {QStringLiteral("temperatureCelsius"), current.temperatureCelsius},
        {QStringLiteral("apparentTemperatureCelsius"), current.apparentTemperatureCelsius},
        {QStringLiteral("relativeHumidityPercent"), current.relativeHumidityPercent},
        {QStringLiteral("precipitationMillimetres"), current.precipitationMillimetres},
        {QStringLiteral("windSpeedKilometresPerHour"), current.windSpeedKilometresPerHour},
        {QStringLiteral("windDirectionDegrees"), current.windDirectionDegrees},
        {QStringLiteral("condition"), static_cast<int>(current.condition)},
        {QStringLiteral("daytime"), current.daytime},
    };
}

std::optional<CurrentWeather> currentFromJson(const QJsonObject &object)
{
    const auto condition = conditionFromJson(object.value(QStringLiteral("condition")));
    if (!condition) {
        return std::nullopt;
    }
    return CurrentWeather{
        .observedAt = QDateTime::fromString(
            object.value(QStringLiteral("observedAt")).toString(), Qt::ISODateWithMs),
        .temperatureCelsius = object.value(
            QStringLiteral("temperatureCelsius")).toDouble(
                std::numeric_limits<double>::quiet_NaN()),
        .apparentTemperatureCelsius = object.value(
            QStringLiteral("apparentTemperatureCelsius")).toDouble(
                std::numeric_limits<double>::quiet_NaN()),
        .relativeHumidityPercent = object.value(
            QStringLiteral("relativeHumidityPercent")).toInt(-1),
        .precipitationMillimetres = object.value(
            QStringLiteral("precipitationMillimetres")).toDouble(-1.0),
        .windSpeedKilometresPerHour = object.value(
            QStringLiteral("windSpeedKilometresPerHour")).toDouble(-1.0),
        .windDirectionDegrees = object.value(
            QStringLiteral("windDirectionDegrees")).toInt(-1),
        .condition = *condition,
        .daytime = object.value(QStringLiteral("daytime")).toBool(false),
    };
}

QJsonObject dayToJson(const DailyWeather &day)
{
    return {
        {QStringLiteral("date"), day.date.toString(Qt::ISODate)},
        {QStringLiteral("minimumTemperatureCelsius"), day.minimumTemperatureCelsius},
        {QStringLiteral("maximumTemperatureCelsius"), day.maximumTemperatureCelsius},
        {QStringLiteral("precipitationProbabilityPercent"), day.precipitationProbabilityPercent},
        {QStringLiteral("precipitationMillimetres"), day.precipitationMillimetres},
        {QStringLiteral("condition"), static_cast<int>(day.condition)},
        {QStringLiteral("sunrise"), day.sunrise.toString(Qt::ISODateWithMs)},
        {QStringLiteral("sunset"), day.sunset.toString(Qt::ISODateWithMs)},
    };
}

std::optional<DailyWeather> dayFromJson(const QJsonObject &object)
{
    const auto condition = conditionFromJson(object.value(QStringLiteral("condition")));
    if (!condition) {
        return std::nullopt;
    }
    return DailyWeather{
        .date = QDate::fromString(
            object.value(QStringLiteral("date")).toString(), Qt::ISODate),
        .minimumTemperatureCelsius = object.value(
            QStringLiteral("minimumTemperatureCelsius")).toDouble(
                std::numeric_limits<double>::quiet_NaN()),
        .maximumTemperatureCelsius = object.value(
            QStringLiteral("maximumTemperatureCelsius")).toDouble(
                std::numeric_limits<double>::quiet_NaN()),
        .precipitationProbabilityPercent = object.value(
            QStringLiteral("precipitationProbabilityPercent")).toInt(-1),
        .precipitationMillimetres = object.value(
            QStringLiteral("precipitationMillimetres")).toDouble(-1.0),
        .condition = *condition,
        .sunrise = QDateTime::fromString(
            object.value(QStringLiteral("sunrise")).toString(), Qt::ISODateWithMs),
        .sunset = QDateTime::fromString(
            object.value(QStringLiteral("sunset")).toString(), Qt::ISODateWithMs),
    };
}

} // namespace

JsonWeatherForecastCache::JsonWeatherForecastCache(QString filePath)
    : m_filePath(std::move(filePath))
{
}

std::optional<WeatherForecast> JsonWeatherForecastCache::load(
    const WeatherLocation &requestedLocation)
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt(-1)
        != CacheSchemaVersion) {
        return std::nullopt;
    }

    auto cachedLocation = locationFromJson(
        root.value(QStringLiteral("location")).toObject());
    auto current = currentFromJson(
        root.value(QStringLiteral("current")).toObject());
    if (!cachedLocation || !current
        || !sameLocation(requestedLocation, *cachedLocation)) {
        return std::nullopt;
    }

    std::vector<DailyWeather> daily;
    const QJsonArray dailyArray = root.value(QStringLiteral("daily")).toArray();
    daily.reserve(static_cast<std::size_t>(dailyArray.size()));
    for (const QJsonValue &value : dailyArray) {
        auto day = dayFromJson(value.toObject());
        if (!day) {
            return std::nullopt;
        }
        daily.push_back(std::move(*day));
    }

    QDateTime fetchedAtUtc = QDateTime::fromString(
        root.value(QStringLiteral("fetchedAtUtc")).toString(),
        Qt::ISODateWithMs);
    if (fetchedAtUtc.isValid()) {
        fetchedAtUtc = fetchedAtUtc.toUTC();
    }
    return WeatherForecast::create(WeatherForecastData{
        .location = std::move(*cachedLocation),
        .current = std::move(*current),
        .daily = std::move(daily),
        .fetchedAtUtc = std::move(fetchedAtUtc),
    });
}

bool JsonWeatherForecastCache::save(const WeatherForecast &forecast)
{
    const QFileInfo fileInfo(m_filePath);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        return false;
    }

    QJsonArray daily;
    for (const DailyWeather &day : forecast.daily()) {
        daily.push_back(dayToJson(day));
    }
    const QJsonObject root{
        {QStringLiteral("schemaVersion"), CacheSchemaVersion},
        {QStringLiteral("location"), locationToJson(forecast.location())},
        {QStringLiteral("current"), currentToJson(forecast.current())},
        {QStringLiteral("daily"), daily},
        {QStringLiteral("fetchedAtUtc"), forecast.fetchedAtUtc().toString(Qt::ISODateWithMs)},
    };

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Compact)) < 0) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

} // namespace stukit::weather
