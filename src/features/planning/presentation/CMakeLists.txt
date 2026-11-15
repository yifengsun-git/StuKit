add_library(StuKitPlanningPresentation STATIC
    PlanningViewModel.cpp
    PlanningViewModel.h
)

target_include_directories(StuKitPlanningPresentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitPlanningPresentation
    PUBLIC
        Qt6::Core
        StuKitPlanningApplication
)

target_compile_features(StuKitPlanningPresentation PUBLIC cxx_std_20)
