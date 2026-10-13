#include "WeatherModule.h"

#include "GetWeatherForecast.h"
#include "JsonWeatherForecastCache.h"
#include "OpenMeteoWeatherService.h"
#include "SearchWeatherLocations.h"
#include "WeatherViewModel.h"

#include <QNetworkAccessManager>
#include <QDir>
#include <QSettings>

#include <utility>

namespace stukit::app {

WeatherModule::WeatherModule(QString appDataDirectory, QSettings &settings)
    : m_appDataDirectory(std::move(appDataDirectory))
    , m_settings(settings)
{
}
WeatherModule::~WeatherModule() = default;

bool WeatherModule::initialize()
{
    m_lastError.clear();
    m_viewModel.reset();
    m_getForecast.reset();
    m_searchLocations.reset();
    m_service.reset();
    m_cache.reset();
    m_network.reset();

    m_network = std::make_unique<QNetworkAccessManager>();
    m_service = std::make_unique<weather::OpenMeteoWeatherService>(*m_network);
    m_cache = std::make_unique<weather::JsonWeatherForecastCache>(
        QDir(m_appDataDirectory).filePath(
            QStringLiteral("weather-cache-v1.json")));
    m_searchLocations = std::make_unique<weather::SearchWeatherLocations>(*m_service);
    m_getForecast = std::make_unique<weather::GetWeatherForecast>(*m_service);
    m_viewModel = std::make_unique<weather::presentation::WeatherViewModel>(
        *m_searchLocations, *m_getForecast, m_settings, m_cache.get());
    m_viewModel->initialize();
    return true;
}

weather::presentation::WeatherViewModel *WeatherModule::viewModel() const noexcept
{
    return m_viewModel.get();
}

const QString &WeatherModule::lastError() const noexcept { return m_lastError; }

} // namespace stukit::app
