add_library(StuKitTasksInfrastructure STATIC
    categories/SqliteTaskCategoryStore.cpp
    categories/SqliteTaskCategoryStore.h
    repositories/SqliteTaskRepository.cpp
    repositories/SqliteTaskRepository.h
    reminders/SqliteReminderDeliveryStore.cpp
    reminders/SqliteReminderDeliveryStore.h
    schedule/ScheduleCourseLookup.cpp
    schedule/ScheduleCourseLookup.h
)

target_include_directories(StuKitTasksInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/repositories
        ${CMAKE_CURRENT_SOURCE_DIR}/categories
        ${CMAKE_CURRENT_SOURCE_DIR}/reminders
        ${CMAKE_CURRENT_SOURCE_DIR}/schedule
)

target_link_libraries(StuKitTasksInfrastructure
    PUBLIC
        Qt6::Sql
        StuKitTasksApplication
        StuKitScheduleDomain
)

target_compile_features(StuKitTasksInfrastructure
    PUBLIC
        cxx_std_20
)
