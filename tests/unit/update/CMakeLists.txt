add_executable(StuKitUpdateTest
    UpdateTest.cpp
)

target_link_libraries(StuKitUpdateTest
    PRIVATE
        Qt6::Test
        StuKitUpdateApplication
        StuKitUpdateInfrastructure
)

add_test(
    NAME update.version_and_release_parser
    COMMAND StuKitUpdateTest
)
