#pragma once

#include <QString>

#include <memory>

class QNetworkAccessManager;
class QSettings;

namespace stukit::weather {
class GetWeatherForecast;
class JsonWeatherForecastCache;
class QWeatherWeatherService;
class SearchWeatherLocations;
namespace presentation { class WeatherViewModel; }
}

namespace stukit::app {

class WeatherModule final
{
public:
    WeatherModule(QString appDataDirectory, QSettings &settings);
    ~WeatherModule();

    WeatherModule(const WeatherModule &) = delete;
    WeatherModule &operator=(const WeatherModule &) = delete;

    [[nodiscard]] bool initialize();
    [[nodiscard]] weather::presentation::WeatherViewModel *viewModel() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    QString m_appDataDirectory;
    QSettings &m_settings;
    QString m_lastError;
    std::unique_ptr<QNetworkAccessManager> m_network;
    std::unique_ptr<weather::JsonWeatherForecastCache> m_cache;
    std::unique_ptr<weather::QWeatherWeatherService> m_service;
    std::unique_ptr<weather::SearchWeatherLocations> m_searchLocations;
    std::unique_ptr<weather::GetWeatherForecast> m_getForecast;
    std::unique_ptr<weather::presentation::WeatherViewModel> m_viewModel;
};

} // namespace stukit::app
