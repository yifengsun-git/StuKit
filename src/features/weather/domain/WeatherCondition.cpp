#include "WeatherCondition.h"

namespace stukit::weather {

WeatherCondition conditionFromWmoCode(int code) noexcept
{
    switch (code) {
    case 0:
        return WeatherCondition::Clear;
    case 1:
        return WeatherCondition::MainlyClear;
    case 2:
        return WeatherCondition::PartlyCloudy;
    case 3:
        return WeatherCondition::Overcast;
    case 45:
    case 48:
        return WeatherCondition::Fog;
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
        return WeatherCondition::Drizzle;
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
        return WeatherCondition::Rain;
    case 71:
    case 73:
    case 75:
    case 77:
        return WeatherCondition::Snow;
    case 80:
    case 81:
    case 82:
        return WeatherCondition::RainShowers;
    case 85:
    case 86:
        return WeatherCondition::SnowShowers;
    case 95:
    case 96:
    case 99:
        return WeatherCondition::Thunderstorm;
    default:
        return WeatherCondition::Unknown;
    }
}

} // namespace stukit::weather
