add_library(StuKitAppCore STATIC
    AccountingUpdatePreparation.cpp
    AccountingUpdatePreparation.h
    AccountingModule.cpp
    AccountingModule.h
    DataMaintenanceViewModel.cpp
    DataMaintenanceViewModel.h
    ScheduleModule.cpp
    ScheduleModule.h
    TaskModule.cpp
    TaskModule.h
    UpdateModule.cpp
    UpdateModule.h
    UserSettings.cpp
    UserSettings.h
    WeatherModule.cpp
    WeatherModule.h
)

target_include_directories(StuKitAppCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAppCore
    PUBLIC
        Qt6::Core
        StuKitAccountingInfrastructure
        StuKitAccountingPresentation
        StuKitScheduleInfrastructure
        StuKitSchedulePresentation
        StuKitTasksInfrastructure
        StuKitTasksPresentation
        StuKitUpdateInfrastructure
        StuKitUpdatePresentation
        StuKitWeatherInfrastructure
        StuKitWeatherPresentation
)

target_compile_features(StuKitAppCore
    PUBLIC
        cxx_std_20
)

target_compile_definitions(StuKitAppCore
    PUBLIC
        STUKIT_VERSION="${PROJECT_VERSION}"
)

set(STUKIT_QWEATHER_API_HOST "" CACHE STRING
    "QWeather project API Host used when no runtime environment value exists")
set(STUKIT_QWEATHER_API_KEY "" CACHE STRING
    "QWeather API Key used when no runtime environment value exists")

configure_file(
    QWeatherBuildConfig.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/QWeatherBuildConfig.h
    @ONLY
)

target_include_directories(StuKitAppCore
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)
