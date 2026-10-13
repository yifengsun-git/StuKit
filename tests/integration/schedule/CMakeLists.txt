add_executable(StuKitSqliteAcademicTermRepositoryTest
    SqliteAcademicTermRepositoryTest.cpp
    ScheduleSqliteTestSupport.h
)

target_link_libraries(StuKitSqliteAcademicTermRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitScheduleInfrastructure
        StuKitAccountingInfrastructure
)

add_test(
    NAME schedule.sqlite_academic_term_repository
    COMMAND StuKitSqliteAcademicTermRepositoryTest
)

add_executable(StuKitSqliteCourseRepositoryTest
    SqliteCourseRepositoryTest.cpp
    ScheduleSqliteTestSupport.h
)

target_link_libraries(StuKitSqliteCourseRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitScheduleInfrastructure
        StuKitAccountingInfrastructure
)

add_test(
    NAME schedule.sqlite_course_repository
    COMMAND StuKitSqliteCourseRepositoryTest
)

add_executable(StuKitSqliteCourseMeetingRepositoryTest
    SqliteCourseMeetingRepositoryTest.cpp
    ScheduleSqliteTestSupport.h
)

target_link_libraries(StuKitSqliteCourseMeetingRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitScheduleInfrastructure
        StuKitAccountingInfrastructure
)

add_test(
    NAME schedule.sqlite_course_meeting_repository
    COMMAND StuKitSqliteCourseMeetingRepositoryTest
)

add_executable(StuKitCsvScheduleImportServiceTest
    CsvScheduleImportServiceTest.cpp
    ScheduleSqliteTestSupport.h
)

target_link_libraries(StuKitCsvScheduleImportServiceTest
    PRIVATE
        Qt6::Test
        StuKitScheduleInfrastructure
        StuKitAccountingInfrastructure
)

add_test(
    NAME schedule.csv_import
    COMMAND StuKitCsvScheduleImportServiceTest
)

add_executable(StuKitSqlitePeriodDefinitionServiceTest
    SqlitePeriodDefinitionServiceTest.cpp
    ScheduleSqliteTestSupport.h
)

target_link_libraries(StuKitSqlitePeriodDefinitionServiceTest
    PRIVATE
        Qt6::Test
        StuKitScheduleInfrastructure
        StuKitAccountingInfrastructure
)

add_test(
    NAME schedule.sqlite_period_definitions
    COMMAND StuKitSqlitePeriodDefinitionServiceTest
)
