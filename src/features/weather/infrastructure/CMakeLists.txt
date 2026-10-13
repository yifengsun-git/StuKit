add_library(StuKitWeatherInfrastructure STATIC
    openmeteo/OpenMeteoJsonParser.cpp
    openmeteo/OpenMeteoJsonParser.h
    openmeteo/OpenMeteoUrlBuilder.cpp
    openmeteo/OpenMeteoUrlBuilder.h
    openmeteo/OpenMeteoWeatherService.cpp
    openmeteo/OpenMeteoWeatherService.h
)

target_include_directories(StuKitWeatherInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/openmeteo
)

target_link_libraries(StuKitWeatherInfrastructure
    PUBLIC
        Qt6::Core
        Qt6::Network
        StuKitWeatherApplication
)

target_compile_features(StuKitWeatherInfrastructure
    PUBLIC
        cxx_std_20
)
