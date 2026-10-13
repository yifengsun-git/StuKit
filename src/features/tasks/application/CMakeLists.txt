add_library(StuKitTasksApplication STATIC
    ChangeTaskStatus.cpp
    ChangeTaskStatus.h
    DeleteTask.cpp
    DeleteTask.h
    GetTaskOverview.cpp
    GetTaskOverview.h
    ICourseLookup.h
    ITaskCategoryStore.h
    IReminderDeliveryStore.h
    ITaskReminderNotifier.h
    ScanTaskReminders.cpp
    ScanTaskReminders.h
    SaveTask.cpp
    SaveTask.h
    TaskReminder.h
)

target_include_directories(StuKitTasksApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitTasksApplication
    PUBLIC
        Qt6::Core
        StuKitTasksDomain
)

target_compile_features(StuKitTasksApplication
    PUBLIC
        cxx_std_20
)
