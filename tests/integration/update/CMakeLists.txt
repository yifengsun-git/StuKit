add_executable(StuKitUpdateServiceIntegrationTest
    GitHubReleaseUpdateServiceTest.cpp
)

target_link_libraries(StuKitUpdateServiceIntegrationTest
    PRIVATE
        Qt6::Network
        Qt6::Test
        StuKitUpdateInfrastructure
)

add_test(
    NAME update.github_release_service
    COMMAND StuKitUpdateServiceIntegrationTest
)
