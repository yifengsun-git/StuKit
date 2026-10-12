add_library(StuKitSchedulePresentation STATIC
    ScheduleManagementModels.cpp
    ScheduleManagementModels.h
    ScheduleManagementViewModel.cpp
    ScheduleManagementViewModel.h
    ScheduleViewModel.cpp
    ScheduleViewModel.h
    WeeklyScheduleListModel.cpp
    WeeklyScheduleListModel.h
)

target_include_directories(StuKitSchedulePresentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitSchedulePresentation
    PUBLIC
        Qt6::Core
        StuKitScheduleApplication
)

target_compile_features(StuKitSchedulePresentation
    PUBLIC
        cxx_std_20
)
