#include "WeatherViewModel.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>
#include <QVariantMap>

#include <memory>
#include <optional>
#include <utility>

using namespace stukit::weather;
using namespace stukit::weather::presentation;

namespace {

WeatherLocation location()
{
    return *WeatherLocation::create(WeatherLocationData{
        .name = QStringLiteral("上海"),
        .country = QStringLiteral("中国"),
        .administrativeArea = QStringLiteral("上海"),
        .timezoneId = QStringLiteral("Asia/Shanghai"),
        .latitude = 31.2304,
        .longitude = 121.4737,
    });
}

WeatherForecast sevenDayForecast(const WeatherLocation &place)
{
    const QTimeZone zone(place.timezoneId().toUtf8());
    const QDate firstDate(2026, 10, 13);
    std::vector<DailyWeather> daily;
    for (int offset = 0; offset < 7; ++offset) {
        const QDate date = firstDate.addDays(offset);
        daily.push_back(DailyWeather{
            .date = date,
            .minimumTemperatureCelsius = 16.0 + offset,
            .maximumTemperatureCelsius = 24.0 + offset,
            .precipitationProbabilityPercent = offset * 5,
            .precipitationMillimetres = 0.0,
            .condition = offset == 0
                ? WeatherCondition::Clear
                : WeatherCondition::PartlyCloudy,
            .sunrise = QDateTime(date, QTime(5, 55), zone),
            .sunset = QDateTime(date, QTime(17, 25), zone),
        });
    }
    return *WeatherForecast::create(WeatherForecastData{
        .location = place,
        .current = CurrentWeather{
            .observedAt = QDateTime(firstDate, QTime(10, 0), zone),
            .temperatureCelsius = 22.0,
            .apparentTemperatureCelsius = 22.5,
            .relativeHumidityPercent = 60,
            .precipitationMillimetres = 0.0,
            .windSpeedKilometresPerHour = 8.0,
            .windDirectionDegrees = 120,
            .condition = WeatherCondition::Clear,
            .daytime = true,
        },
        .daily = std::move(daily),
        .fetchedAtUtc = QDateTime(
            firstDate, QTime(2, 5), QTimeZone::UTC),
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

struct Fixture
{
    Fixture()
        : directory(QDir(QCoreApplication::applicationDirPath()).filePath(
              QStringLiteral("weather-qml-test-XXXXXX")))
        , settings(
              directory.filePath(QStringLiteral("settings.ini")),
              QSettings::IniFormat)
        , search(searchService)
        , getForecast(forecastService)
        , viewModel(search, getForecast, settings)
    {
        Q_ASSERT(directory.isValid());
    }

    QTemporaryDir directory;
    QSettings settings;
    FakeLocationSearchService searchService;
    FakeForecastService forecastService;
    SearchWeatherLocations search;
    GetWeatherForecast getForecast;
    WeatherViewModel viewModel;
};

std::unique_ptr<QObject> createWeatherPage(
    QQmlEngine &engine,
    WeatherViewModel &viewModel,
    qreal width,
    qreal height = 720)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitWeatherTest"),
        QStringLiteral("WeatherHomePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }

    QVariantMap properties{
        {QStringLiteral("viewModel"),
         QVariant::fromValue(static_cast<QObject *>(&viewModel))},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
    };
    std::unique_ptr<QObject> page(
        component.createWithInitialProperties(properties));
    if (!page) {
        qWarning().noquote() << component.errorString();
        return {};
    }
    QCoreApplication::processEvents();
    return page;
}

} // namespace

class WeatherQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void showsEmptyStateAndResponsiveLayout();
    void searchesSelectsAndRendersForecast();
};

void WeatherQmlIntegrationTest::showsEmptyStateAndResponsiveLayout()
{
    Fixture fixture;
    QQmlEngine engine;
    auto page = createWeatherPage(engine, fixture.viewModel, 360, 640);
    QVERIFY(page);

    QCOMPARE(page->property("compactLayout").toBool(), true);
    QObject *searchField = page->findChild<QObject *>(
        QStringLiteral("weatherCitySearchField"));
    QObject *searchButton = page->findChild<QObject *>(
        QStringLiteral("weatherSearchButton"));
    QObject *emptyState = page->findChild<QObject *>(
        QStringLiteral("weatherEmptyState"));
    QObject *metricGrid = page->findChild<QObject *>(
        QStringLiteral("weatherMetricGrid"));
    QObject *forecastGrid = page->findChild<QObject *>(
        QStringLiteral("dailyForecastGrid"));
    QVERIFY(searchField);
    QVERIFY(searchButton);
    QVERIFY(emptyState);
    QVERIFY(metricGrid);
    QVERIFY(forecastGrid);
    QVERIFY(page->property("emptyStateShown").toBool());
    QCOMPARE(metricGrid->property("columns").toInt(), 2);
    QCOMPARE(forecastGrid->property("columns").toInt(), 1);
    QVERIFY(searchButton->property("implicitHeight").toReal() >= 44.0);

    page->setProperty("width", 960);
    QTRY_COMPARE(page->property("compactLayout").toBool(), false);
    QTRY_COMPARE(metricGrid->property("columns").toInt(), 3);
}

void WeatherQmlIntegrationTest::searchesSelectsAndRendersForecast()
{
    Fixture fixture;
    QQmlEngine engine;
    auto page = createWeatherPage(engine, fixture.viewModel, 960);
    QVERIFY(page);

    QObject *searchField = page->findChild<QObject *>(
        QStringLiteral("weatherCitySearchField"));
    QObject *searchButton = page->findChild<QObject *>(
        QStringLiteral("weatherSearchButton"));
    QVERIFY(searchField);
    QVERIFY(searchButton);
    searchField->setProperty("text", QStringLiteral("上海"));
    QCoreApplication::processEvents();
    QVERIFY(searchButton->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(searchButton, "click"));
    QCOMPARE(fixture.searchService.lastQuery, QStringLiteral("上海"));

    fixture.searchService.complete(
        std::vector<WeatherLocation>{location()});
    QCoreApplication::processEvents();
    QObject *resultsRepeater = page->findChild<QObject *>(
        QStringLiteral("weatherSearchResultsRepeater"));
    QVERIFY(resultsRepeater);
    QCOMPARE(resultsRepeater->property("count").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(
        page.get(),
        "selectSearchResult",
        Q_ARG(QVariant, QVariant(0))));
    QCOMPARE(fixture.forecastService.callCount, 1);
    QVERIFY(fixture.forecastService.lastLocation.has_value());

    fixture.forecastService.complete(sevenDayForecast(
        *fixture.forecastService.lastLocation));
    QCoreApplication::processEvents();

    QObject *currentCard = page->findChild<QObject *>(
        QStringLiteral("currentWeatherCard"));
    QObject *forecastGrid = page->findChild<QObject *>(
        QStringLiteral("dailyForecastGrid"));
    QObject *attribution = page->findChild<QObject *>(
        QStringLiteral("weatherAttributionLabel"));
    QObject *refreshButton = page->findChild<QObject *>(
        QStringLiteral("weatherRefreshButton"));
    QVERIFY(currentCard);
    QVERIFY(forecastGrid);
    QVERIFY(attribution);
    QVERIFY(refreshButton);
    QVERIFY(page->property("forecastShown").toBool());
    QCOMPARE(forecastGrid->property("forecastCount").toInt(), 7);
    QCOMPARE(forecastGrid->property("columns").toInt(), 7);
    QVERIFY(!attribution->property("text").toString().isEmpty());

    page->setProperty("width", 360);
    QTRY_COMPARE(forecastGrid->property("columns").toInt(), 1);
    QVERIFY(QMetaObject::invokeMethod(refreshButton, "click"));
    QCOMPARE(fixture.forecastService.callCount, 2);
    fixture.forecastService.complete(
        WeatherServiceError::NetworkUnavailable);
    QCoreApplication::processEvents();

    QObject *errorLabel = page->findChild<QObject *>(
        QStringLiteral("weatherErrorLabel"));
    QVERIFY(errorLabel);
    QVERIFY(page->property("errorShown").toBool());
    QVERIFY(page->property("forecastShown").toBool());
}

QTEST_MAIN(WeatherQmlIntegrationTest)

#include "WeatherQmlIntegrationTest.moc"
