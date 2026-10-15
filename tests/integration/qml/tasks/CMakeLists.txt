add_executable(StuKitTasksQmlIntegrationTest
    ../TasksQmlIntegrationTest.cpp
)

set(STUKIT_TASKS_TEST_QML_FILES
    ${CMAKE_SOURCE_DIR}/src/features/tasks/qml/TasksHomePage.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppComboBox.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppDialog.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppSectionCard.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextArea.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextField.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/DateTimePickerDialog.qml
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/tasks/qml/TasksHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS TasksHomePage.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    PROPERTIES QT_RESOURCE_ALIAS AppButton.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppComboBox.qml
    PROPERTIES QT_RESOURCE_ALIAS AppComboBox.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS AppDialog.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppSectionCard.qml
    PROPERTIES QT_RESOURCE_ALIAS AppSectionCard.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextArea.qml
    PROPERTIES QT_RESOURCE_ALIAS AppTextArea.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextField.qml
    PROPERTIES QT_RESOURCE_ALIAS AppTextField.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/DateTimePickerDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS DateTimePickerDialog.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

qt_add_qml_module(StuKitTasksQmlIntegrationTest
    URI StuKitTasksTest
    VERSION 1.0
    QML_FILES
        ${STUKIT_TASKS_TEST_QML_FILES}
)

target_include_directories(StuKitTasksQmlIntegrationTest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/tasks
)

target_link_libraries(StuKitTasksQmlIntegrationTest
    PRIVATE
        Qt6::Quick
        Qt6::Test
        StuKitTasksPresentation
)

add_test(
    NAME app.tasks_qml
    COMMAND StuKitTasksQmlIntegrationTest
)

set_tests_properties(app.tasks_qml PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_FONTDIR=C:/Windows/Fonts;QT_QUICK_CONTROLS_STYLE=Basic"
)
