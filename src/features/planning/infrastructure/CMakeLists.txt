add_library(StuKitPlanningInfrastructure STATIC
    ai/AnthropicLanguageModelService.cpp
    ai/AnthropicLanguageModelService.h
    ai/OpenAiCompatibleLanguageModelService.cpp
    ai/OpenAiCompatibleLanguageModelService.h
    ai/RoutingLanguageModelService.cpp
    ai/RoutingLanguageModelService.h
    files/TextSourceFileReader.cpp
    files/TextSourceFileReader.h
    files/MarkdownPlanFileWriter.cpp
    files/MarkdownPlanFileWriter.h
    repositories/InMemoryPlanningProjectRepository.cpp
    repositories/InMemoryPlanningProjectRepository.h
    repositories/InMemoryPlanRevisionRepository.cpp
    repositories/InMemoryPlanRevisionRepository.h
    repositories/InMemorySourceDocumentRepository.cpp
    repositories/InMemorySourceDocumentRepository.h
    repositories/SqlitePlanningProjectRepository.cpp
    repositories/SqlitePlanningProjectRepository.h
    repositories/SqlitePlanRevisionRepository.cpp
    repositories/SqlitePlanRevisionRepository.h
    repositories/SqliteSourceDocumentRepository.cpp
    repositories/SqliteSourceDocumentRepository.h
    secrets/WindowsCredentialSecretStore.cpp
    secrets/WindowsCredentialSecretStore.h
)

target_include_directories(StuKitPlanningInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/files
        ${CMAKE_CURRENT_SOURCE_DIR}/ai
        ${CMAKE_CURRENT_SOURCE_DIR}/repositories
        ${CMAKE_CURRENT_SOURCE_DIR}/secrets
)

target_link_libraries(StuKitPlanningInfrastructure
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        StuKitPlanningApplication
)

target_compile_features(StuKitPlanningInfrastructure PUBLIC cxx_std_20)

if(WIN32)
    target_link_libraries(StuKitPlanningInfrastructure PUBLIC Advapi32)
endif()
