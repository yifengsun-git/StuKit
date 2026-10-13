#pragma once

namespace stukit::weather {

enum class WeatherCondition
{
    Clear,
    MainlyClear,
    PartlyCloudy,
    Overcast,
    Fog,
    Drizzle,
    Rain,
    Snow,
    RainShowers,
    SnowShowers,
    Thunderstorm,
    Unknown,
};

[[nodiscard]] WeatherCondition conditionFromWmoCode(int code) noexcept;

} // namespace stukit::weather
