add_executable(StuKitScheduleQmlIntegrationTest
    ../ScheduleQmlIntegrationTest.cpp
)

set(STUKIT_SCHEDULE_TEST_QML_FILES
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/ScheduleHomePage.qml
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/CourseMeetingCard.qml
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/DayScheduleList.qml
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/ScheduleGridEditor.qml
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/ScheduleManagementPanel.qml
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/WeeklyGrid.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppComboBox.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppDialog.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppSectionCard.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextArea.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextField.qml
)

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/ScheduleHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleHomePage.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/CourseMeetingCard.qml
    PROPERTIES QT_RESOURCE_ALIAS CourseMeetingCard.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/DayScheduleList.qml
    PROPERTIES QT_RESOURCE_ALIAS DayScheduleList.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/ScheduleGridEditor.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleGridEditor.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/ScheduleManagementPanel.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleManagementPanel.qml
)
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/features/schedule/qml/components/WeeklyGrid.qml
    PROPERTIES QT_RESOURCE_ALIAS WeeklyGrid.qml
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
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

qt_add_qml_module(StuKitScheduleQmlIntegrationTest
    URI StuKitScheduleTest
    VERSION 1.0
    QML_FILES
        ${STUKIT_SCHEDULE_TEST_QML_FILES}
)

target_include_directories(StuKitScheduleQmlIntegrationTest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/unit/schedule
)

target_link_libraries(StuKitScheduleQmlIntegrationTest
    PRIVATE
        Qt6::Quick
        Qt6::Test
        StuKitScheduleApplication
        StuKitScheduleDomain
        StuKitSchedulePresentation
)

add_test(
    NAME app.schedule_qml
    COMMAND StuKitScheduleQmlIntegrationTest
)

set_tests_properties(app.schedule_qml PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_FONTDIR=C:/Windows/Fonts;QT_QUICK_CONTROLS_STYLE=Basic"
)
