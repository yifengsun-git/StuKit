#pragma once

#include "GetWeatherForecast.h"
#include "IWeatherForecastCache.h"
#include "SearchWeatherLocations.h"

#include <QObject>
#include <QVariantList>

#include <optional>
#include <vector>

class QSettings;

namespace stukit::weather::presentation {

class WeatherViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool searching READ searching NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool hasForecast READ hasForecast NOTIFY forecastChanged)
    Q_PROPERTY(bool usingCachedForecast READ usingCachedForecast NOTIFY forecastChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(QVariantList dailyForecasts READ dailyForecasts NOTIFY forecastChanged)
    Q_PROPERTY(QString locationName READ locationName NOTIFY locationChanged)
    Q_PROPERTY(QString locationSubtitle READ locationSubtitle NOTIFY locationChanged)
    Q_PROPERTY(QString temperatureText READ temperatureText NOTIFY forecastChanged)
    Q_PROPERTY(QString apparentTemperatureText READ apparentTemperatureText NOTIFY forecastChanged)
    Q_PROPERTY(QString conditionText READ conditionText NOTIFY forecastChanged)
    Q_PROPERTY(QString humidityText READ humidityText NOTIFY forecastChanged)
    Q_PROPERTY(QString precipitationText READ precipitationText NOTIFY forecastChanged)
    Q_PROPERTY(QString windText READ windText NOTIFY forecastChanged)
    Q_PROPERTY(QString updatedText READ updatedText NOTIFY forecastChanged)

public:
    WeatherViewModel(
        SearchWeatherLocations &searchLocations,
        GetWeatherForecast &getForecast,
        QSettings &settings,
        IWeatherForecastCache *cache = nullptr,
        QObject *parent = nullptr);
    ~WeatherViewModel() override;

    void initialize();

    [[nodiscard]] bool searching() const noexcept;
    [[nodiscard]] bool loading() const noexcept;
    [[nodiscard]] bool hasForecast() const noexcept;
    [[nodiscard]] bool usingCachedForecast() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] QVariantList searchResults() const;
    [[nodiscard]] QVariantList dailyForecasts() const;
    [[nodiscard]] QString locationName() const;
    [[nodiscard]] QString locationSubtitle() const;
    [[nodiscard]] QString temperatureText() const;
    [[nodiscard]] QString apparentTemperatureText() const;
    [[nodiscard]] QString conditionText() const;
    [[nodiscard]] QString humidityText() const;
    [[nodiscard]] QString precipitationText() const;
    [[nodiscard]] QString windText() const;
    [[nodiscard]] QString updatedText() const;

    Q_INVOKABLE void search(const QString &query);
    Q_INVOKABLE bool selectLocation(int index);
    Q_INVOKABLE void refresh();

signals:
    void stateChanged();
    void searchResultsChanged();
    void locationChanged();
    void forecastChanged();

private:
    void setError(QString message);
    void persistLocation();
    void restoreCachedForecast();
    [[nodiscard]] static QString conditionName(WeatherCondition condition);
    [[nodiscard]] static QString errorText(WeatherServiceError error);

    SearchWeatherLocations &m_searchLocations;
    GetWeatherForecast &m_getForecast;
    QSettings &m_settings;
    IWeatherForecastCache *m_cache;
    bool m_searching{false};
    bool m_loading{false};
    QString m_errorMessage;
    std::vector<WeatherLocation> m_searchResults;
    std::optional<WeatherLocation> m_selectedLocation;
    std::optional<WeatherForecast> m_forecast;
    bool m_usingCachedForecast{false};
};

} // namespace stukit::weather::presentation
