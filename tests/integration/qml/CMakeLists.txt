add_executable(StuKitAnalyticsQmlIntegrationTest
    AnalyticsQmlIntegrationTest.cpp
)

set(STUKIT_ANALYTICS_TEST_QML_FILES
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/AnalyticsHomePage.qml
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/AnalyticsCard.qml
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/AnalyticsFilterPanel.qml
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/BarSeriesChart.qml
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/DonutBreakdown.qml
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/LineSeriesChart.qml
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/RankingBars.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/AnalyticsHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS AnalyticsHomePage.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/AnalyticsCard.qml
    PROPERTIES QT_RESOURCE_ALIAS AnalyticsCard.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/AnalyticsFilterPanel.qml
    PROPERTIES QT_RESOURCE_ALIAS AnalyticsFilterPanel.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/BarSeriesChart.qml
    PROPERTIES QT_RESOURCE_ALIAS BarSeriesChart.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/DonutBreakdown.qml
    PROPERTIES QT_RESOURCE_ALIAS DonutBreakdown.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/LineSeriesChart.qml
    PROPERTIES QT_RESOURCE_ALIAS LineSeriesChart.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/analytics/qml/components/RankingBars.qml
    PROPERTIES QT_RESOURCE_ALIAS RankingBars.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    PROPERTIES QT_RESOURCE_ALIAS AppButton.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

qt_add_qml_module(StuKitAnalyticsQmlIntegrationTest
    URI StuKitAnalyticsTest
    VERSION 1.0
    QML_FILES
        ${STUKIT_ANALYTICS_TEST_QML_FILES}
)

target_include_directories(StuKitAnalyticsQmlIntegrationTest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/accounting
)

target_link_libraries(StuKitAnalyticsQmlIntegrationTest
    PRIVATE
        Qt6::Charts
        Qt6::Quick
        Qt6::Test
        Qt6::Widgets
        StuKitAccountingApplication
        StuKitAccountingDomain
        StuKitAccountingPresentation
)

add_test(
    NAME app.analytics_qml
    COMMAND StuKitAnalyticsQmlIntegrationTest
)

set_tests_properties(app.analytics_qml PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_FONTDIR=C:/Windows/Fonts"
)

add_subdirectory(schedule)
add_subdirectory(weather)
add_subdirectory(tasks)
