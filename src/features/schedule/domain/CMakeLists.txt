add_library(StuKitScheduleDomain STATIC
    AcademicTerm.cpp
    AcademicTerm.h
    Course.cpp
    Course.h
    CourseMeeting.cpp
    CourseMeeting.h
    IAcademicTermRepository.h
    ICourseMeetingRepository.h
    ICourseRepository.h
    Weekday.h
    WeekPattern.h
)

target_include_directories(StuKitScheduleDomain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitScheduleDomain
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitScheduleDomain
    PUBLIC
        cxx_std_20
)
