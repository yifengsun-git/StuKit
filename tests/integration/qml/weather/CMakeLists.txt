add_executable(StuKitWeatherQmlIntegrationTest
    ../WeatherQmlIntegrationTest.cpp
)

set(STUKIT_WEATHER_TEST_QML_FILES
    ${CMAKE_SOURCE_DIR}/src/features/weather/qml/WeatherHomePage.qml
    ${CMAKE_SOURCE_DIR}/src/features/weather/qml/components/CurrentWeatherCard.qml
    ${CMAKE_SOURCE_DIR}/src/features/weather/qml/components/DailyForecastGrid.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppSectionCard.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextField.qml
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/weather/qml/WeatherHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS WeatherHomePage.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/weather/qml/components/CurrentWeatherCard.qml
    PROPERTIES QT_RESOURCE_ALIAS CurrentWeatherCard.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/weather/qml/components/DailyForecastGrid.qml
    PROPERTIES QT_RESOURCE_ALIAS DailyForecastGrid.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    PROPERTIES QT_RESOURCE_ALIAS AppButton.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppSectionCard.qml
    PROPERTIES QT_RESOURCE_ALIAS AppSectionCard.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextField.qml
    PROPERTIES QT_RESOURCE_ALIAS AppTextField.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

qt_add_qml_module(StuKitWeatherQmlIntegrationTest
    URI StuKitWeatherTest
    VERSION 1.0
    QML_FILES
        ${STUKIT_WEATHER_TEST_QML_FILES}
)

target_link_libraries(StuKitWeatherQmlIntegrationTest
    PRIVATE
        Qt6::Quick
        Qt6::Test
        StuKitWeatherPresentation
)

add_test(
    NAME app.weather_qml
    COMMAND StuKitWeatherQmlIntegrationTest
)

set_tests_properties(app.weather_qml PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_FONTDIR=C:/Windows/Fonts;QT_QUICK_CONTROLS_STYLE=Basic"
)
