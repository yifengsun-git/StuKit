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

add_executable(StuKitSqliteCategoryRepositoryTest
    SqliteCategoryRepositoryTest.cpp
)

target_link_libraries(StuKitSqliteCategoryRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
        StuKitAccountingInfrastructure
)

add_test(
    NAME accounting.sqlite_category_repository
    COMMAND StuKitSqliteCategoryRepositoryTest
)

add_executable(StuKitSqliteTransactionRepositoryTest
    SqliteTransactionRepositoryTest.cpp
)

target_link_libraries(StuKitSqliteTransactionRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
        StuKitAccountingInfrastructure
)

add_test(
    NAME accounting.sqlite_transaction_repository
    COMMAND StuKitSqliteTransactionRepositoryTest
)
