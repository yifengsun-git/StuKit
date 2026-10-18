add_library(StuKitUpdateApplication STATIC
    UpdateVersion.cpp
    UpdateVersion.h
)

target_include_directories(StuKitUpdateApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitUpdateApplication
    PUBLIC
        Qt6::Core
        StuKitUpdateDomain
)

target_compile_features(StuKitUpdateApplication PUBLIC cxx_std_20)
