add_library(StuKitAnalyticsPresentation STATIC
    ChartSeriesListModel.cpp
    ChartSeriesListModel.h
)

target_include_directories(StuKitAnalyticsPresentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAnalyticsPresentation
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitAnalyticsPresentation
    PUBLIC
        cxx_std_20
)
