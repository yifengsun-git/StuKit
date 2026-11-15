add_library(StuKitPlanningApplication STATIC
    ILanguageModelService.h
    IPlanFileWriter.h
    IPlanningActionPublisher.h
    ISecretStore.h
    ISourceFileReader.h
    PlanningWorkspace.cpp
    PlanningWorkspace.h
    PlanningAnalysis.cpp
    PlanningAnalysis.h
)

target_include_directories(StuKitPlanningApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitPlanningApplication
    PUBLIC
        Qt6::Core
        StuKitPlanningDomain
)

target_compile_features(StuKitPlanningApplication PUBLIC cxx_std_20)
