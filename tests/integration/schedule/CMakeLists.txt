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
