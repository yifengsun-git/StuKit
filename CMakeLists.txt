cmake_minimum_required(VERSION 3.21)

project(StuKit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.8)

include(CTest)

add_subdirectory(src/features/accounting/domain)

if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

qt_add_executable(StuKit
    src/app/main.cpp
)

set(STUKIT_QML_FILES
    src/shell/qml/Main.qml
    src/shell/qml/HomePage.qml
    src/features/accounting/qml/AccountingHomePage.qml
    src/features/schedule/qml/ScheduleHomePage.qml
    src/shared/qml/theme/AppTheme.qml
    src/shared/qml/components/FeaturePlaceholder.qml
)

set_source_files_properties(
    src/shell/qml/Main.qml
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

set_source_files_properties(
    src/shell/qml/HomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS HomePage.qml
)

set_source_files_properties(
    src/features/accounting/qml/AccountingHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS AccountingHomePage.qml
)

set_source_files_properties(
    src/features/schedule/qml/ScheduleHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleHomePage.qml
)

set_source_files_properties(
    src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

set_source_files_properties(
    src/shared/qml/components/FeaturePlaceholder.qml
    PROPERTIES QT_RESOURCE_ALIAS FeaturePlaceholder.qml
)

qt_add_qml_module(StuKit
    URI StuKit
    VERSION 1.0
    QML_FILES
        ${STUKIT_QML_FILES}
)

target_link_libraries(StuKit
    PRIVATE Qt6::Quick
)
