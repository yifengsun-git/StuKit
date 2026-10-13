add_executable(StuKitSqliteTaskRepositoryTest
    SqliteTaskRepositoryTest.cpp
)

target_link_libraries(StuKitSqliteTaskRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingInfrastructure
        StuKitTasksInfrastructure
)

add_test(
    NAME tasks.sqlite_repository
    COMMAND StuKitSqliteTaskRepositoryTest
)
