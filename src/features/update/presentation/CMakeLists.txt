add_library(StuKitUpdatePresentation STATIC
    UpdateViewModel.cpp
    UpdateViewModel.h
)

target_include_directories(StuKitUpdatePresentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitUpdatePresentation
    PUBLIC
        Qt6::Core
        Qt6::Gui
        StuKitUpdateApplication
)

target_compile_features(StuKitUpdatePresentation PUBLIC cxx_std_20)
