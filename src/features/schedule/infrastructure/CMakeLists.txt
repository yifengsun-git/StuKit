add_library(StuKitScheduleInfrastructure STATIC
    import/CsvScheduleImportService.cpp
    import/CsvScheduleImportService.h
    repositories/SqliteAcademicTermRepository.cpp
    repositories/SqliteAcademicTermRepository.h
    repositories/SqliteCourseMeetingRepository.cpp
    repositories/SqliteCourseMeetingRepository.h
    repositories/SqliteCourseRepository.cpp
    repositories/SqliteCourseRepository.h
    settings/SqlitePeriodDefinitionService.cpp
    settings/SqlitePeriodDefinitionService.h
)

target_include_directories(StuKitScheduleInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitScheduleInfrastructure
    PUBLIC
        Qt6::Sql
        StuKitScheduleApplication
)

target_compile_features(StuKitScheduleInfrastructure
    PUBLIC
        cxx_std_20
)
