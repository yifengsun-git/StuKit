add_library(StuKitScheduleApplication STATIC
    DetectScheduleConflicts.cpp
    DetectScheduleConflicts.h
    GetAcademicTerms.cpp
    GetAcademicTerms.h
    GetWeeklySchedule.cpp
    GetWeeklySchedule.h
    SaveAcademicTerm.cpp
    SaveAcademicTerm.h
    SaveCourse.cpp
    SaveCourse.h
    SaveCourseMeeting.cpp
    SaveCourseMeeting.h
)

target_include_directories(StuKitScheduleApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitScheduleApplication
    PUBLIC
        StuKitScheduleDomain
)

target_compile_features(StuKitScheduleApplication
    PUBLIC
        cxx_std_20
)
