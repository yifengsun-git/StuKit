add_executable(StuKitSqliteDatabaseTest
    SqliteDatabaseTest.cpp
)

target_link_libraries(StuKitSqliteDatabaseTest
    PRIVATE
        Qt6::Test
        StuKitAccountingInfrastructure
)

add_test(
    NAME accounting.sqlite_database
    COMMAND StuKitSqliteDatabaseTest
)

add_executable(StuKitSqliteAccountRepositoryTest
    SqliteAccountRepositoryTest.cpp
)

target_link_libraries(StuKitSqliteAccountRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
        StuKitAccountingInfrastructure
)

add_test(
    NAME accounting.sqlite_account_repository
    COMMAND StuKitSqliteAccountRepositoryTest
)
