add_executable(StuKitPlanningDomainTest
    PlanningDomainTest.cpp
)

target_link_libraries(StuKitPlanningDomainTest
    PRIVATE
        Qt6::Test
        StuKitPlanningDomain
)

add_test(
    NAME planning.domain
    COMMAND StuKitPlanningDomainTest
)

add_executable(StuKitPlanningWorkspaceTest
    PlanningWorkspaceTest.cpp
)

target_link_libraries(StuKitPlanningWorkspaceTest
    PRIVATE
        Qt6::Test
        StuKitPlanningInfrastructure
        StuKitPlanningPresentation
)

add_test(
    NAME planning.workspace
    COMMAND StuKitPlanningWorkspaceTest
)
