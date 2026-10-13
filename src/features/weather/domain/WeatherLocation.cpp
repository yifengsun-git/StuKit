#include "WeatherLocation.h"

#include <QTimeZone>

#include <cmath>
#include <utility>

namespace stukit::weather {
namespace {

bool isValidOptionalText(const QString &text)
{
    return text.trimmed().size() <= WeatherLocation::MaximumTextLength;
}

} // namespace

std::optional<WeatherLocation> WeatherLocation::create(
    WeatherLocationData data)
{
    data.name = data.name.trimmed();
    data.country = data.country.trimmed();
    data.administrativeArea = data.administrativeArea.trimmed();
    data.timezoneId = data.timezoneId.trimmed();

    const bool validCoordinate = std::isfinite(data.latitude)
        && std::isfinite(data.longitude)
        && data.latitude >= -90.0
        && data.latitude <= 90.0
        && data.longitude >= -180.0
        && data.longitude <= 180.0;
    const bool validName = !data.name.isEmpty()
        && data.name.size() <= MaximumTextLength;
    const bool validTimeZone = QTimeZone(data.timezoneId.toUtf8()).isValid();

    if (!validCoordinate
        || !validName
        || !isValidOptionalText(data.country)
        || !isValidOptionalText(data.administrativeArea)
        || !validTimeZone) {
        return std::nullopt;
    }

    return WeatherLocation(std::move(data));
}

WeatherLocation::WeatherLocation(WeatherLocationData data)
    : m_data(std::move(data))
{
}

const QString &WeatherLocation::name() const noexcept { return m_data.name; }

const QString &WeatherLocation::country() const noexcept
{
    return m_data.country;
}

const QString &WeatherLocation::administrativeArea() const noexcept
{
    return m_data.administrativeArea;
}

const QString &WeatherLocation::timezoneId() const noexcept
{
    return m_data.timezoneId;
}

double WeatherLocation::latitude() const noexcept { return m_data.latitude; }

double WeatherLocation::longitude() const noexcept { return m_data.longitude; }

} // namespace stukit::weather
