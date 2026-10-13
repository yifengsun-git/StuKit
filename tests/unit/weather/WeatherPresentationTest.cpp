#include "WeatherViewModel.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

#include <optional>
#include <utility>

using namespace stukit::weather;
using namespace stukit::weather::presentation;

namespace {

WeatherLocation location(QString name = QStringLiteral("上海"))
{
    return *WeatherLocation::create(WeatherLocationData{
        .name = std::move(name),
        .country = QStringLiteral("中国"),
        .administrativeArea = QStringLiteral("上海"),
        .timezoneId = QStringLiteral("Asia/Shanghai"),
        .latitude = 31.2304,
        .longitude = 121.4737,
    });
}

WeatherForecast forecastFor(const WeatherLocation &place)
{
    const QTimeZone zone(place.timezoneId().toUtf8());
    return *WeatherForecast::create(WeatherForecastData{
        .location = place,
        .current = CurrentWeather{
            .observedAt = QDateTime(QDate(2026, 10, 13), QTime(10, 0), zone),
            .temperatureCelsius = 22.0,
            .apparentTemperatureCelsius = 22.5,
            .relativeHumidityPercent = 60,
            .precipitationMillimetres = 0.0,
            .windSpeedKilometresPerHour = 8.0,
            .windDirectionDegrees = 120,
            .condition = WeatherCondition::Clear,
            .daytime = true,
        },
        .daily = {DailyWeather{
            .date = QDate(2026, 10, 13),
            .minimumTemperatureCelsius = 16.0,
            .maximumTemperatureCelsius = 24.0,
            .precipitationProbabilityPercent = 10,
            .precipitationMillimetres = 0.0,
            .condition = WeatherCondition::Clear,
            .sunrise = QDateTime(QDate(2026, 10, 13), QTime(5, 55), zone),
            .sunset = QDateTime(QDate(2026, 10, 13), QTime(17, 25), zone),
        }},
        .fetchedAtUtc = QDateTime(
            QDate(2026, 10, 13), QTime(2, 5), QTimeZone::UTC),
    });
}

class FakeLocationSearchService final : public ILocationSearchService
{
public:
    WeatherRequestId search(
        QString query,
        LocationSearchCompletion completion) override
    {
        ++callCount;
        lastQuery = std::move(query);
        pending = std::move(completion);
        return static_cast<WeatherRequestId>(callCount);
    }

    void cancel(WeatherRequestId) override { pending = {}; }

    void complete(LocationSearchResult result)
    {
        auto completion = std::move(pending);
        completion(std::move(result));
    }

    int callCount{0};
    QString lastQuery;
    LocationSearchCompletion pending;
};

class FakeForecastService final : public IWeatherForecastService
{
public:
    WeatherRequestId fetch(
        WeatherLocation place,
        ForecastCompletion completion) override
    {
        ++callCount;
        lastLocation = std::move(place);
        pending = std::move(completion);
        return static_cast<WeatherRequestId>(callCount);
    }

    void cancel(WeatherRequestId) override { pending = {}; }

    void complete(ForecastResult result)
    {
        auto completion = std::move(pending);
        completion(std::move(result));
    }

    int callCount{0};
    std::optional<WeatherLocation> lastLocation;
    ForecastCompletion pending;
};

class FakeForecastCache final : public IWeatherForecastCache
{
public:
    std::optional<WeatherForecast> load(
        const WeatherLocation &place) override
    {
        ++loadCount;
        loadedFor = place;
        return stored;
    }

    bool save(const WeatherForecast &forecast) override
    {
        ++saveCount;
        stored = forecast;
        return true;
    }

    int loadCount{0};
    int saveCount{0};
    std::optional<WeatherLocation> loadedFor;
    std::optional<WeatherForecast> stored;
};

} // namespace

class WeatherPresentationTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsInvalidSearch();
    void selectsPersistsAndRestoresLocation();
    void keepsForecastWhenRefreshFails();
    void restoresCacheThenReplacesItWithFreshForecast();
};

void WeatherPresentationTest::rejectsInvalidSearch()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    FakeLocationSearchService searchService;
    FakeForecastService forecastService;
    SearchWeatherLocations search(searchService);
    GetWeatherForecast getForecast(forecastService);
    WeatherViewModel viewModel(search, getForecast, settings);

    viewModel.search(QStringLiteral("a"));

    QCOMPARE(searchService.callCount, 0);
    QVERIFY(!viewModel.searching());
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.errorMessage(),
             QStringLiteral("请输入至少两个字符的城市名"));
}

void WeatherPresentationTest::selectsPersistsAndRestoresLocation()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString settingsPath = directory.filePath(QStringLiteral("settings.ini"));
    QSettings settings(settingsPath, QSettings::IniFormat);

    {
        FakeLocationSearchService searchService;
        FakeForecastService forecastService;
        SearchWeatherLocations search(searchService);
        GetWeatherForecast getForecast(forecastService);
        WeatherViewModel viewModel(search, getForecast, settings);

        viewModel.search(QStringLiteral("  上海  "));
        QVERIFY(viewModel.searching());
        QCOMPARE(searchService.lastQuery, QStringLiteral("上海"));
        searchService.complete(std::vector<WeatherLocation>{location()});
        QCOMPARE(viewModel.searchResults().size(), 1);

        QVERIFY(viewModel.selectLocation(0));
        QVERIFY(viewModel.loading());
        QCOMPARE(viewModel.locationName(), QStringLiteral("上海"));
        QVERIFY(forecastService.lastLocation.has_value());
        QCOMPARE(forecastService.lastLocation->name(), QStringLiteral("上海"));
        forecastService.complete(forecastFor(*forecastService.lastLocation));

        QVERIFY(viewModel.hasForecast());
        QCOMPARE(viewModel.temperatureText(), QStringLiteral("22.0 °C"));
        QCOMPARE(viewModel.dailyForecasts().size(), 1);
        QCOMPARE(settings.value(QStringLiteral("weather/location/name")).toString(),
                 QStringLiteral("上海"));
    }

    FakeLocationSearchService restoredSearchService;
    FakeForecastService restoredForecastService;
    SearchWeatherLocations restoredSearch(restoredSearchService);
    GetWeatherForecast restoredForecast(restoredForecastService);
    WeatherViewModel restoredViewModel(
        restoredSearch, restoredForecast, settings);

    restoredViewModel.initialize();

    QCOMPARE(restoredViewModel.locationName(), QStringLiteral("上海"));
    QCOMPARE(restoredForecastService.callCount, 1);
    QVERIFY(restoredViewModel.loading());
}

void WeatherPresentationTest::keepsForecastWhenRefreshFails()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    FakeLocationSearchService searchService;
    FakeForecastService forecastService;
    SearchWeatherLocations search(searchService);
    GetWeatherForecast getForecast(forecastService);
    WeatherViewModel viewModel(search, getForecast, settings);

    viewModel.search(QStringLiteral("上海"));
    searchService.complete(std::vector<WeatherLocation>{location()});
    QVERIFY(viewModel.selectLocation(0));
    forecastService.complete(forecastFor(*forecastService.lastLocation));
    QVERIFY(viewModel.hasForecast());

    viewModel.refresh();
    forecastService.complete(WeatherServiceError::NetworkUnavailable);

    QVERIFY(viewModel.hasForecast());
    QVERIFY(viewModel.hasError());
    QCOMPARE(viewModel.temperatureText(), QStringLiteral("22.0 °C"));
    QCOMPARE(viewModel.errorMessage(), QStringLiteral("网络不可用，请稍后重试"));
}

void WeatherPresentationTest::restoresCacheThenReplacesItWithFreshForecast()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("settings.ini")),
                       QSettings::IniFormat);
    const WeatherLocation place = location();
    settings.setValue(QStringLiteral("weather/location/name"), place.name());
    settings.setValue(QStringLiteral("weather/location/country"), place.country());
    settings.setValue(QStringLiteral("weather/location/administrativeArea"),
                      place.administrativeArea());
    settings.setValue(QStringLiteral("weather/location/timezoneId"),
                      place.timezoneId());
    settings.setValue(QStringLiteral("weather/location/latitude"),
                      place.latitude());
    settings.setValue(QStringLiteral("weather/location/longitude"),
                      place.longitude());

    FakeLocationSearchService searchService;
    FakeForecastService forecastService;
    FakeForecastCache cache;
    cache.stored = forecastFor(place);
    SearchWeatherLocations search(searchService);
    GetWeatherForecast getForecast(forecastService);
    WeatherViewModel viewModel(
        search, getForecast, settings, &cache);

    viewModel.initialize();

    QCOMPARE(cache.loadCount, 1);
    QVERIFY(viewModel.hasForecast());
    QVERIFY(viewModel.usingCachedForecast());
    QVERIFY(viewModel.loading());

    forecastService.complete(WeatherServiceError::NetworkUnavailable);
    QVERIFY(viewModel.hasForecast());
    QVERIFY(viewModel.usingCachedForecast());

    viewModel.refresh();
    forecastService.complete(forecastFor(place));
    QCOMPARE(cache.saveCount, 1);
    QVERIFY(!viewModel.usingCachedForecast());
    QVERIFY(!viewModel.hasError());
}

QTEST_APPLESS_MAIN(WeatherPresentationTest)

#include "WeatherPresentationTest.moc"
