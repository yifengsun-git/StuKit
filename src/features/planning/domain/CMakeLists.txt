add_library(StuKitPlanningDomain STATIC
    IPlanningProjectRepository.h
    IPlanRevisionRepository.h
    ISourceDocumentRepository.h
    PlanRevision.cpp
    PlanRevision.h
    PlanningProject.cpp
    PlanningProject.h
    PlanningTypes.h
    SourceDocument.cpp
    SourceDocument.h
)

target_include_directories(StuKitPlanningDomain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitPlanningDomain
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitPlanningDomain
    PUBLIC
        cxx_std_20
)
