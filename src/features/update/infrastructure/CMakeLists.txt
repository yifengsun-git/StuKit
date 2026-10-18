add_library(StuKitUpdateInfrastructure STATIC
    github/GitHubReleaseParser.cpp
    github/GitHubReleaseParser.h
    github/GitHubReleaseUpdateService.cpp
    github/GitHubReleaseUpdateService.h
    platform/WindowsUpdateInstaller.cpp
    platform/WindowsUpdateInstaller.h
)

target_include_directories(StuKitUpdateInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/github
        ${CMAKE_CURRENT_SOURCE_DIR}/platform
)

target_link_libraries(StuKitUpdateInfrastructure
    PUBLIC
        Qt6::Core
        Qt6::Network
        StuKitUpdateApplication
)

target_compile_features(StuKitUpdateInfrastructure PUBLIC cxx_std_20)
