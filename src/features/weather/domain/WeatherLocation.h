#pragma once

#include <QString>

#include <optional>

namespace stukit::weather {

struct WeatherLocationData
{
    QString name;
    QString country;
    QString administrativeArea;
    QString timezoneId;
    double latitude{0.0};
    double longitude{0.0};
};

class WeatherLocation final
{
public:
    static constexpr qsizetype MaximumTextLength = 100;

    static std::optional<WeatherLocation> create(WeatherLocationData data);

    [[nodiscard]] const QString &name() const noexcept;
    [[nodiscard]] const QString &country() const noexcept;
    [[nodiscard]] const QString &administrativeArea() const noexcept;
    [[nodiscard]] const QString &timezoneId() const noexcept;
    [[nodiscard]] double latitude() const noexcept;
    [[nodiscard]] double longitude() const noexcept;

private:
    explicit WeatherLocation(WeatherLocationData data);

    WeatherLocationData m_data;
};

} // namespace stukit::weather
