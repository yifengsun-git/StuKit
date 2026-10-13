#pragma once

#include "WeatherCondition.h"
#include "WeatherLocation.h"

#include <QDate>
#include <QDateTime>

#include <optional>
#include <vector>

namespace stukit::weather {

struct CurrentWeather
{
    QDateTime observedAt;
    double temperatureCelsius{0.0};
    double apparentTemperatureCelsius{0.0};
    int relativeHumidityPercent{0};
    double precipitationMillimetres{0.0};
    double windSpeedKilometresPerHour{0.0};
    int windDirectionDegrees{0};
    WeatherCondition condition{WeatherCondition::Unknown};
    bool daytime{false};
};

struct DailyWeather
{
    QDate date;
    double minimumTemperatureCelsius{0.0};
    double maximumTemperatureCelsius{0.0};
    int precipitationProbabilityPercent{0};
    double precipitationMillimetres{0.0};
    WeatherCondition condition{WeatherCondition::Unknown};
    QDateTime sunrise;
    QDateTime sunset;
};

struct WeatherForecastData
{
    WeatherLocation location;
    CurrentWeather current;
    std::vector<DailyWeather> daily;
    QDateTime fetchedAtUtc;
};

class WeatherForecast final
{
public:
    static constexpr int MaximumForecastDays = 16;

    static std::optional<WeatherForecast> create(WeatherForecastData data);

    [[nodiscard]] const WeatherLocation &location() const noexcept;
    [[nodiscard]] const CurrentWeather &current() const noexcept;
    [[nodiscard]] const std::vector<DailyWeather> &daily() const noexcept;
    [[nodiscard]] const QDateTime &fetchedAtUtc() const noexcept;

private:
    explicit WeatherForecast(WeatherForecastData data);

    WeatherForecastData m_data;
};

} // namespace stukit::weather
