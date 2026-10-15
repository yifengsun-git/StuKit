#include "WeatherViewModel.h"

#include <QLocale>
#include <QSettings>
#include <QStringList>
#include <QTimeZone>
#include <QVariantMap>

#include <utility>

namespace stukit::weather::presentation {
namespace {

QString decimal(double value, int precision = 1)
{
    return QLocale::c().toString(value, 'f', precision);
}

QString locationDetails(const WeatherLocation &location)
{
    QStringList parts;
    if (!location.administrativeArea().isEmpty()
        && location.administrativeArea() != location.name()) {
        parts.push_back(location.administrativeArea());
    }
    if (!location.country().isEmpty()) {
        parts.push_back(location.country());
    }
    return parts.join(QStringLiteral(" · "));
}

} // namespace

WeatherViewModel::WeatherViewModel(
    SearchWeatherLocations &searchLocations,
    GetWeatherForecast &getForecast,
    QSettings &settings,
    IWeatherForecastCache *cache,
    QObject *parent)
    : QObject(parent)
    , m_searchLocations(searchLocations)
    , m_getForecast(getForecast)
    , m_settings(settings)
    , m_cache(cache)
{
}

WeatherViewModel::~WeatherViewModel()
{
    m_searchLocations.cancel();
    m_getForecast.cancel();
}

void WeatherViewModel::initialize()
{
    auto location = WeatherLocation::create(WeatherLocationData{
        .name = m_settings.value(QStringLiteral("weather/location/name")).toString(),
        .country = m_settings.value(QStringLiteral("weather/location/country")).toString(),
        .administrativeArea = m_settings.value(
            QStringLiteral("weather/location/administrativeArea")).toString(),
        .timezoneId = m_settings.value(
            QStringLiteral("weather/location/timezoneId")).toString(),
        .latitude = m_settings.value(
            QStringLiteral("weather/location/latitude")).toDouble(),
        .longitude = m_settings.value(
            QStringLiteral("weather/location/longitude")).toDouble(),
    });
    if (location.has_value()) {
        m_selectedLocation = std::move(*location);
        emit locationChanged();
        restoreCachedForecast();
        refresh();
    }
}

bool WeatherViewModel::searching() const noexcept { return m_searching; }
bool WeatherViewModel::loading() const noexcept { return m_loading; }
bool WeatherViewModel::hasForecast() const noexcept { return m_forecast.has_value(); }
bool WeatherViewModel::usingCachedForecast() const noexcept
{
    return m_usingCachedForecast;
}
bool WeatherViewModel::hasError() const noexcept { return !m_errorMessage.isEmpty(); }
const QString &WeatherViewModel::errorMessage() const noexcept { return m_errorMessage; }

QVariantList WeatherViewModel::searchResults() const
{
    QVariantList result;
    for (qsizetype index = 0;
         index < static_cast<qsizetype>(m_searchResults.size()); ++index) {
        const WeatherLocation &location = m_searchResults[static_cast<std::size_t>(index)];
        result.push_back(QVariantMap{
            {QStringLiteral("index"), index},
            {QStringLiteral("name"), location.name()},
            {QStringLiteral("subtitle"), locationDetails(location)},
            {QStringLiteral("timezone"), location.timezoneId()},
        });
    }
    return result;
}

QVariantList WeatherViewModel::dailyForecasts() const
{
    QVariantList result;
    if (!m_forecast.has_value()) {
        return result;
    }
    for (const DailyWeather &day : m_forecast->daily()) {
        result.push_back(QVariantMap{
            {QStringLiteral("date"), day.date.toString(QStringLiteral("MM-dd"))},
            {QStringLiteral("weekday"), QLocale(QLocale::Chinese).dayName(
                 day.date.dayOfWeek(), QLocale::ShortFormat)},
            {QStringLiteral("condition"), conditionName(day.condition)},
            {QStringLiteral("minimum"), decimal(day.minimumTemperatureCelsius) + QStringLiteral("°")},
            {QStringLiteral("maximum"), decimal(day.maximumTemperatureCelsius) + QStringLiteral("°")},
            {QStringLiteral("precipitationProbability"),
             QString::number(day.precipitationProbabilityPercent) + QStringLiteral("%")},
        });
    }
    return result;
}

QString WeatherViewModel::locationName() const
{
    return m_selectedLocation ? m_selectedLocation->name() : QString{};
}

QString WeatherViewModel::locationSubtitle() const
{
    return m_selectedLocation ? locationDetails(*m_selectedLocation) : QString{};
}

QString WeatherViewModel::temperatureText() const
{
    return m_forecast ? decimal(m_forecast->current().temperatureCelsius) + QStringLiteral(" °C") : QString{};
}

QString WeatherViewModel::apparentTemperatureText() const
{
    return m_forecast ? QStringLiteral("体感 %1 °C").arg(decimal(
        m_forecast->current().apparentTemperatureCelsius)) : QString{};
}

QString WeatherViewModel::conditionText() const
{
    return m_forecast ? conditionName(m_forecast->current().condition) : QString{};
}

QString WeatherViewModel::humidityText() const
{
    return m_forecast ? QStringLiteral("%1%").arg(
        m_forecast->current().relativeHumidityPercent) : QString{};
}

QString WeatherViewModel::precipitationText() const
{
    return m_forecast ? QStringLiteral("%1 mm").arg(decimal(
        m_forecast->current().precipitationMillimetres)) : QString{};
}

QString WeatherViewModel::windText() const
{
    return m_forecast ? QStringLiteral("%1 km/h · %2°")
        .arg(decimal(m_forecast->current().windSpeedKilometresPerHour))
        .arg(m_forecast->current().windDirectionDegrees) : QString{};
}

QString WeatherViewModel::updatedText() const
{
    if (!m_forecast) {
        return {};
    }
    const QTimeZone zone(m_forecast->location().timezoneId().toUtf8());
    return QStringLiteral("更新于 %1").arg(
        m_forecast->fetchedAtUtc().toTimeZone(zone).toString(
            QStringLiteral("MM-dd HH:mm")));
}

void WeatherViewModel::search(const QString &query)
{
    m_searching = true;
    m_errorMessage.clear();
    emit stateChanged();
    m_searchLocations.execute(query, [this](LocationSearchResult result) {
        m_searching = false;
        if (std::holds_alternative<WeatherServiceError>(result)) {
            m_searchResults.clear();
            setError(errorText(std::get<WeatherServiceError>(result)));
        } else {
            m_searchResults = std::get<std::vector<WeatherLocation>>(std::move(result));
            m_errorMessage = m_searchResults.empty()
                ? QStringLiteral("没有找到匹配的城市") : QString{};
            emit stateChanged();
        }
        emit searchResultsChanged();
    });
}

bool WeatherViewModel::selectLocation(int index)
{
    if (index < 0 || index >= static_cast<int>(m_searchResults.size())) {
        setError(QStringLiteral("所选城市不存在"));
        return false;
    }
    m_selectedLocation = m_searchResults[static_cast<std::size_t>(index)];
    m_searchResults.clear();
    m_forecast.reset();
    m_usingCachedForecast = false;
    m_errorMessage.clear();
    persistLocation();
    emit locationChanged();
    emit searchResultsChanged();
    emit forecastChanged();
    emit stateChanged();
    restoreCachedForecast();
    refresh();
    return true;
}

void WeatherViewModel::refresh()
{
    if (!m_selectedLocation) {
        setError(QStringLiteral("请先搜索并选择城市"));
        return;
    }
    m_loading = true;
    m_errorMessage.clear();
    emit stateChanged();
    m_getForecast.execute(*m_selectedLocation, [this](ForecastResult result) {
        m_loading = false;
        if (std::holds_alternative<WeatherServiceError>(result)) {
            setError(errorText(std::get<WeatherServiceError>(result)));
            return;
        }
        m_forecast = std::get<WeatherForecast>(std::move(result));
        m_usingCachedForecast = false;
        if (m_cache) {
            static_cast<void>(m_cache->save(*m_forecast));
        }
        m_errorMessage.clear();
        emit forecastChanged();
        emit stateChanged();
    });
}

void WeatherViewModel::restoreCachedForecast()
{
    if (!m_cache || !m_selectedLocation) {
        return;
    }
    auto cached = m_cache->load(*m_selectedLocation);
    if (!cached) {
        return;
    }
    m_forecast = std::move(*cached);
    m_usingCachedForecast = true;
    emit forecastChanged();
}

void WeatherViewModel::setError(QString message)
{
    m_errorMessage = std::move(message);
    emit stateChanged();
}

void WeatherViewModel::persistLocation()
{
    if (!m_selectedLocation) return;
    m_settings.setValue(QStringLiteral("weather/location/name"), m_selectedLocation->name());
    m_settings.setValue(QStringLiteral("weather/location/country"), m_selectedLocation->country());
    m_settings.setValue(QStringLiteral("weather/location/administrativeArea"), m_selectedLocation->administrativeArea());
    m_settings.setValue(QStringLiteral("weather/location/timezoneId"), m_selectedLocation->timezoneId());
    m_settings.setValue(QStringLiteral("weather/location/latitude"), m_selectedLocation->latitude());
    m_settings.setValue(QStringLiteral("weather/location/longitude"), m_selectedLocation->longitude());
}

QString WeatherViewModel::conditionName(WeatherCondition condition)
{
    switch (condition) {
    case WeatherCondition::Clear: return QStringLiteral("晴");
    case WeatherCondition::MainlyClear: return QStringLiteral("大部晴朗");
    case WeatherCondition::PartlyCloudy: return QStringLiteral("多云");
    case WeatherCondition::Overcast: return QStringLiteral("阴");
    case WeatherCondition::Fog: return QStringLiteral("雾");
    case WeatherCondition::Drizzle: return QStringLiteral("毛毛雨");
    case WeatherCondition::Rain: return QStringLiteral("雨");
    case WeatherCondition::Snow: return QStringLiteral("雪");
    case WeatherCondition::RainShowers: return QStringLiteral("阵雨");
    case WeatherCondition::SnowShowers: return QStringLiteral("阵雪");
    case WeatherCondition::Thunderstorm: return QStringLiteral("雷暴");
    case WeatherCondition::Unknown: return QStringLiteral("未知天气");
    }
    return QStringLiteral("未知天气");
}

QString WeatherViewModel::errorText(WeatherServiceError error)
{
    switch (error) {
    case WeatherServiceError::InvalidInput: return QStringLiteral("请输入至少两个字符的城市名");
    case WeatherServiceError::ConfigurationMissing: return QStringLiteral("请先配置和风天气 API Host 与 API Key");
    case WeatherServiceError::AuthenticationFailed: return QStringLiteral("和风天气认证失败，请检查 API Host 与 API Key");
    case WeatherServiceError::NetworkUnavailable: return QStringLiteral("网络不可用，请稍后重试");
    case WeatherServiceError::RateLimited: return QStringLiteral("天气服务请求过于频繁，请稍后重试");
    case WeatherServiceError::ServiceUnavailable: return QStringLiteral("天气服务暂时不可用");
    case WeatherServiceError::InvalidResponse: return QStringLiteral("天气服务返回了无法识别的数据");
    case WeatherServiceError::Cancelled: return QStringLiteral("请求已取消");
    }
    return QStringLiteral("天气请求失败");
}

} // namespace stukit::weather::presentation
