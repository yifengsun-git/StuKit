#include "WeatherForecast.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace stukit::weather {
namespace {

bool validTemperature(double value)
{
    return std::isfinite(value) && value >= -100.0 && value <= 70.0;
}

bool validNonNegative(double value, double maximum)
{
    return std::isfinite(value) && value >= 0.0 && value <= maximum;
}

bool validCurrent(const CurrentWeather &weather)
{
    return weather.observedAt.isValid()
        && validTemperature(weather.temperatureCelsius)
        && validTemperature(weather.apparentTemperatureCelsius)
        && weather.relativeHumidityPercent >= 0
        && weather.relativeHumidityPercent <= 100
        && validNonNegative(weather.precipitationMillimetres, 1000.0)
        && validNonNegative(weather.windSpeedKilometresPerHour, 500.0)
        && weather.windDirectionDegrees >= 0
        && weather.windDirectionDegrees <= 360;
}

bool validDay(const DailyWeather &weather)
{
    return weather.date.isValid()
        && validTemperature(weather.minimumTemperatureCelsius)
        && validTemperature(weather.maximumTemperatureCelsius)
        && weather.minimumTemperatureCelsius
            <= weather.maximumTemperatureCelsius
        && weather.precipitationProbabilityPercent >= 0
        && weather.precipitationProbabilityPercent <= 100
        && validNonNegative(weather.precipitationMillimetres, 2000.0)
        && weather.sunrise.isValid()
        && weather.sunset.isValid()
        && weather.sunrise < weather.sunset;
}

} // namespace

std::optional<WeatherForecast> WeatherForecast::create(
    WeatherForecastData data)
{
    if (!validCurrent(data.current)
        || !data.fetchedAtUtc.isValid()
        || data.fetchedAtUtc.timeSpec() != Qt::UTC
        || data.daily.empty()
        || data.daily.size()
            > static_cast<std::size_t>(MaximumForecastDays)
        || !std::ranges::all_of(data.daily, validDay)
        || !std::ranges::is_sorted(
            data.daily,
            {},
            &DailyWeather::date)) {
        return std::nullopt;
    }

    const auto adjacent = std::ranges::adjacent_find(
        data.daily,
        [](const DailyWeather &left, const DailyWeather &right) {
            return left.date == right.date;
        });
    if (adjacent != data.daily.end()) {
        return std::nullopt;
    }

    return WeatherForecast(std::move(data));
}

WeatherForecast::WeatherForecast(WeatherForecastData data)
    : m_data(std::move(data))
{
}

const WeatherLocation &WeatherForecast::location() const noexcept
{
    return m_data.location;
}

const CurrentWeather &WeatherForecast::current() const noexcept
{
    return m_data.current;
}

const std::vector<DailyWeather> &WeatherForecast::daily() const noexcept
{
    return m_data.daily;
}

const QDateTime &WeatherForecast::fetchedAtUtc() const noexcept
{
    return m_data.fetchedAtUtc;
}

} // namespace stukit::weather
