add_executable(StuKitAcademicTermTest
    AcademicTermTest.cpp
)

target_link_libraries(StuKitAcademicTermTest
    PRIVATE
        Qt6::Test
        StuKitScheduleDomain
)

add_test(
    NAME schedule.academic_term
    COMMAND StuKitAcademicTermTest
)

add_executable(StuKitCourseTest
    CourseTest.cpp
)

target_link_libraries(StuKitCourseTest
    PRIVATE
        Qt6::Test
        StuKitScheduleDomain
)

add_test(
    NAME schedule.course
    COMMAND StuKitCourseTest
)

add_executable(StuKitCourseMeetingTest
    CourseMeetingTest.cpp
)

target_link_libraries(StuKitCourseMeetingTest
    PRIVATE
        Qt6::Test
        StuKitScheduleDomain
)

add_test(
    NAME schedule.course_meeting
    COMMAND StuKitCourseMeetingTest
)

add_executable(StuKitScheduleRepositoryTest
    ScheduleRepositoryTest.cpp
    support/InMemoryAcademicTermRepository.h
    support/InMemoryCourseMeetingRepository.h
    support/InMemoryCourseRepository.h
)

target_link_libraries(StuKitScheduleRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitScheduleDomain
)

add_test(
    NAME schedule.repositories
    COMMAND StuKitScheduleRepositoryTest
)

add_executable(StuKitDetectScheduleConflictsTest
    DetectScheduleConflictsTest.cpp
    support/InMemoryAcademicTermRepository.h
    support/InMemoryCourseMeetingRepository.h
    support/InMemoryCourseRepository.h
)

target_link_libraries(StuKitDetectScheduleConflictsTest
    PRIVATE
        Qt6::Test
        StuKitScheduleApplication
)

add_test(
    NAME schedule.detect_conflicts
    COMMAND StuKitDetectScheduleConflictsTest
)
