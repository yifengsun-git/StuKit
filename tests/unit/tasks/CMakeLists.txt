add_executable(StuKitTaskItemTest
    TaskItemTest.cpp
)

target_link_libraries(StuKitTaskItemTest
    PRIVATE
        Qt6::Test
        StuKitTasksDomain
)

add_test(
    NAME tasks.task_item
    COMMAND StuKitTaskItemTest
)

add_executable(StuKitTaskApplicationTest
    TaskApplicationTest.cpp
    support/InMemoryTaskRepository.h
)

target_link_libraries(StuKitTaskApplicationTest
    PRIVATE
        Qt6::Test
        StuKitTasksApplication
)

add_test(
    NAME tasks.application
    COMMAND StuKitTaskApplicationTest
)
