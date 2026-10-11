add_library(StuKitAccountingInfrastructure STATIC
    backup/SqliteBackupService.cpp
    backup/SqliteBackupService.h
    database/SqliteDatabase.cpp
    database/SqliteDatabase.h
    export/CsvTransactionWriter.cpp
    export/CsvTransactionWriter.h
    repositories/SqliteAccountRepository.cpp
    repositories/SqliteAccountRepository.h
    repositories/SqliteCategoryRepository.cpp
    repositories/SqliteCategoryRepository.h
    repositories/SqliteTransactionRepository.cpp
    repositories/SqliteTransactionRepository.h
)

target_include_directories(StuKitAccountingInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingInfrastructure
    PUBLIC
        Qt6::Sql
        StuKitAccountingApplication
        StuKitAccountingDomain
)

target_compile_features(StuKitAccountingInfrastructure
    PUBLIC
        cxx_std_20
)
