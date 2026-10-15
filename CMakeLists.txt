cmake_minimum_required(VERSION 3.21)

project(StuKit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick Sql Charts Widgets Network)
qt_standard_project_setup(REQUIRES 6.10)

include(CTest)

add_subdirectory(src/features/tasks/domain)
add_subdirectory(src/features/tasks/application)
add_subdirectory(src/features/tasks/infrastructure)
add_subdirectory(src/features/tasks/presentation)
add_subdirectory(src/features/schedule/domain)
add_subdirectory(src/features/schedule/application)
add_subdirectory(src/features/schedule/infrastructure)
add_subdirectory(src/features/schedule/presentation)
add_subdirectory(src/features/weather/domain)
add_subdirectory(src/features/weather/application)
add_subdirectory(src/features/weather/infrastructure)
add_subdirectory(src/features/weather/presentation)
add_subdirectory(src/features/accounting/domain)
add_subdirectory(src/features/accounting/application)
add_subdirectory(src/features/accounting/infrastructure)
add_subdirectory(src/features/analytics/presentation)
add_subdirectory(src/features/accounting/presentation)
add_subdirectory(src/app)

qt_add_executable(StuKit
    src/app/main.cpp
)

set(STUKIT_QML_FILES
    src/shell/qml/Main.qml
    src/shell/qml/HomePage.qml
    src/features/analytics/qml/AnalyticsHomePage.qml
    src/features/analytics/qml/components/AnalyticsCard.qml
    src/features/analytics/qml/components/AnalyticsFilterPanel.qml
    src/features/analytics/qml/components/BarSeriesChart.qml
    src/features/analytics/qml/components/DonutBreakdown.qml
    src/features/analytics/qml/components/LineSeriesChart.qml
    src/features/analytics/qml/components/RankingBars.qml
    src/features/accounting/qml/AccountingHomePage.qml
    src/features/schedule/qml/ScheduleHomePage.qml
    src/features/schedule/qml/components/CourseMeetingCard.qml
    src/features/schedule/qml/components/DayScheduleList.qml
    src/features/schedule/qml/components/ScheduleGridEditor.qml
    src/features/schedule/qml/components/ScheduleManagementPanel.qml
    src/features/schedule/qml/components/WeeklyGrid.qml
    src/features/weather/qml/WeatherHomePage.qml
    src/features/weather/qml/components/CurrentWeatherCard.qml
    src/features/weather/qml/components/DailyForecastGrid.qml
    src/features/tasks/qml/TasksHomePage.qml
    src/shared/qml/theme/AppTheme.qml
    src/shared/qml/components/AppButton.qml
    src/shared/qml/components/AppComboBox.qml
    src/shared/qml/components/AppDialog.qml
    src/shared/qml/components/AppNavigationButton.qml
    src/shared/qml/components/AppSectionCard.qml
    src/shared/qml/components/AppTextArea.qml
    src/shared/qml/components/AppTextField.qml
    src/shared/qml/components/DateTimePickerDialog.qml
    src/shared/qml/components/FeatureCard.qml
    src/shared/qml/components/FeaturePlaceholder.qml
)

set_source_files_properties(
    src/shell/qml/Main.qml
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

set_source_files_properties(
    src/shell/qml/HomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS HomePage.qml
)

set_source_files_properties(
    src/features/analytics/qml/AnalyticsHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS AnalyticsHomePage.qml
)

set_source_files_properties(
    src/features/analytics/qml/components/AnalyticsCard.qml
    PROPERTIES QT_RESOURCE_ALIAS AnalyticsCard.qml
)

set_source_files_properties(
    src/features/analytics/qml/components/AnalyticsFilterPanel.qml
    PROPERTIES QT_RESOURCE_ALIAS AnalyticsFilterPanel.qml
)

set_source_files_properties(
    src/features/analytics/qml/components/BarSeriesChart.qml
    PROPERTIES QT_RESOURCE_ALIAS BarSeriesChart.qml
)

set_source_files_properties(
    src/features/analytics/qml/components/DonutBreakdown.qml
    PROPERTIES QT_RESOURCE_ALIAS DonutBreakdown.qml
)

set_source_files_properties(
    src/features/analytics/qml/components/LineSeriesChart.qml
    PROPERTIES QT_RESOURCE_ALIAS LineSeriesChart.qml
)

set_source_files_properties(
    src/features/analytics/qml/components/RankingBars.qml
    PROPERTIES QT_RESOURCE_ALIAS RankingBars.qml
)

set_source_files_properties(
    src/features/accounting/qml/AccountingHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS AccountingHomePage.qml
)

set_source_files_properties(
    src/features/schedule/qml/ScheduleHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleHomePage.qml
)

set_source_files_properties(
    src/features/schedule/qml/components/ScheduleManagementPanel.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleManagementPanel.qml
)

set_source_files_properties(
    src/features/schedule/qml/components/CourseMeetingCard.qml
    PROPERTIES QT_RESOURCE_ALIAS CourseMeetingCard.qml
)

set_source_files_properties(
    src/features/schedule/qml/components/DayScheduleList.qml
    PROPERTIES QT_RESOURCE_ALIAS DayScheduleList.qml
)

set_source_files_properties(
    src/features/schedule/qml/components/ScheduleGridEditor.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleGridEditor.qml
)

set_source_files_properties(
    src/features/schedule/qml/components/WeeklyGrid.qml
    PROPERTIES QT_RESOURCE_ALIAS WeeklyGrid.qml
)

set_source_files_properties(
    src/features/weather/qml/WeatherHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS WeatherHomePage.qml
)

set_source_files_properties(
    src/features/weather/qml/components/CurrentWeatherCard.qml
    PROPERTIES QT_RESOURCE_ALIAS CurrentWeatherCard.qml
)

set_source_files_properties(
    src/features/weather/qml/components/DailyForecastGrid.qml
    PROPERTIES QT_RESOURCE_ALIAS DailyForecastGrid.qml
)

set_source_files_properties(
    src/features/tasks/qml/TasksHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS TasksHomePage.qml
)

set_source_files_properties(
    src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

set_source_files_properties(
    src/shared/qml/components/AppButton.qml
    PROPERTIES QT_RESOURCE_ALIAS AppButton.qml
)

set_source_files_properties(
    src/shared/qml/components/AppComboBox.qml
    PROPERTIES QT_RESOURCE_ALIAS AppComboBox.qml
)

set_source_files_properties(
    src/shared/qml/components/AppDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS AppDialog.qml
)

set_source_files_properties(
    src/shared/qml/components/AppNavigationButton.qml
    PROPERTIES QT_RESOURCE_ALIAS AppNavigationButton.qml
)

set_source_files_properties(
    src/shared/qml/components/AppSectionCard.qml
    PROPERTIES QT_RESOURCE_ALIAS AppSectionCard.qml
)

set_source_files_properties(
    src/shared/qml/components/AppTextArea.qml
    PROPERTIES QT_RESOURCE_ALIAS AppTextArea.qml
)

set_source_files_properties(
    src/shared/qml/components/AppTextField.qml
    PROPERTIES QT_RESOURCE_ALIAS AppTextField.qml
)

set_source_files_properties(
    src/shared/qml/components/DateTimePickerDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS DateTimePickerDialog.qml
)

set_source_files_properties(
    src/shared/qml/components/FeatureCard.qml
    PROPERTIES QT_RESOURCE_ALIAS FeatureCard.qml
)

set_source_files_properties(
    src/shared/qml/components/FeaturePlaceholder.qml
    PROPERTIES QT_RESOURCE_ALIAS FeaturePlaceholder.qml
)

qt_add_qml_module(StuKit
    URI StuKit
    VERSION 1.0
    QML_FILES
        ${STUKIT_QML_FILES}
)

target_link_libraries(StuKit
    PRIVATE
        Qt6::Quick
        Qt6::Charts
        Qt6::Widgets
        StuKitAppCore
)

if(WIN32)
    install(TARGETS StuKit
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES
        README.md
        LICENSE
        DESTINATION .
    )

    qt_generate_deploy_qml_app_script(
        TARGET StuKit
        OUTPUT_SCRIPT STUKIT_DEPLOY_SCRIPT
    )
    install(SCRIPT ${STUKIT_DEPLOY_SCRIPT})
endif()

if(ANDROID)
    set(STUKIT_ANDROID_PACKAGE_NAME
        "io.github.yifengsungit.stukit"
        CACHE STRING "Android application package name")
    set(STUKIT_ANDROID_VERSION_CODE
        "1"
        CACHE STRING "Monotonically increasing Android version code")

    set_target_properties(StuKit PROPERTIES
        QT_ANDROID_APP_NAME "StuKit"
        QT_ANDROID_PACKAGE_NAME "${STUKIT_ANDROID_PACKAGE_NAME}"
        QT_ANDROID_PACKAGE_SOURCE_DIR
            "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_TARGET_SDK_VERSION 36
        QT_ANDROID_VERSION_CODE "${STUKIT_ANDROID_VERSION_CODE}"
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
    )

    qt_add_android_permission(StuKit
        NAME android.permission.INTERNET
    )
endif()

if(BUILD_TESTING AND NOT ANDROID)
    find_package(Qt6 6.10 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
