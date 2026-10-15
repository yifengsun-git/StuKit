add_executable(StuKitShellQmlIntegrationTest
    ../ShellQmlIntegrationTest.cpp
)

set(STUKIT_SHELL_TEST_QML_FILES
    ${CMAKE_SOURCE_DIR}/src/shell/qml/HomePage.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/FeatureCard.qml
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shell/qml/HomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS HomePage.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/FeatureCard.qml
    PROPERTIES QT_RESOURCE_ALIAS FeatureCard.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

qt_add_qml_module(StuKitShellQmlIntegrationTest
    URI StuKitShellTest
    VERSION 1.0
    QML_FILES
        ${STUKIT_SHELL_TEST_QML_FILES}
)

target_link_libraries(StuKitShellQmlIntegrationTest
    PRIVATE
        Qt6::Quick
        Qt6::Test
)

add_test(
    NAME app.shell_qml
    COMMAND StuKitShellQmlIntegrationTest
)

set_tests_properties(app.shell_qml PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_FONTDIR=C:/Windows/Fonts;QT_QUICK_CONTROLS_STYLE=Basic"
)
