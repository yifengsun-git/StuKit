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
