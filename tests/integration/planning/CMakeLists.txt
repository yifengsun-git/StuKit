add_executable(StuKitSqlitePlanningRepositoryTest
    SqlitePlanningRepositoryTest.cpp
)

target_link_libraries(StuKitSqlitePlanningRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingInfrastructure
        StuKitPlanningInfrastructure
)

add_test(
    NAME planning.sqlite_repositories
    COMMAND StuKitSqlitePlanningRepositoryTest
)
