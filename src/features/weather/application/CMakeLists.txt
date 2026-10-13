add_library(StuKitWeatherApplication STATIC
    GetWeatherForecast.cpp
    GetWeatherForecast.h
    ILocationSearchService.h
    IWeatherForecastService.h
    SearchWeatherLocations.cpp
    SearchWeatherLocations.h
    WeatherServiceTypes.h
)

target_include_directories(StuKitWeatherApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitWeatherApplication
    PUBLIC
        Qt6::Core
        StuKitWeatherDomain
)

target_compile_features(StuKitWeatherApplication
    PUBLIC
        cxx_std_20
)
