add_executable(StuKitAccountingHomeQmlIntegrationTest
    ../AccountingHomeQmlIntegrationTest.cpp
)

set(STUKIT_ACCOUNTING_HOME_TEST_QML_FILES
    ${CMAKE_SOURCE_DIR}/src/features/accounting/qml/AccountingHomePage.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppButton.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppComboBox.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppDialog.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppSectionCard.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextArea.qml
    ${CMAKE_SOURCE_DIR}/src/shared/qml/components/AppTextField.qml
)

foreach(QML_FILE IN LISTS STUKIT_ACCOUNTING_HOME_TEST_QML_FILES)
    get_filename_component(QML_FILE_NAME "${QML_FILE}" NAME)
    set_source_files_properties(
        "${QML_FILE}"
        PROPERTIES QT_RESOURCE_ALIAS "${QML_FILE_NAME}"
    )
endforeach()

set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/shared/qml/theme/AppTheme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(StuKitAccountingHomeQmlIntegrationTest
    URI StuKitAccountingHomeTest
    VERSION 1.0
    QML_FILES
        ${STUKIT_ACCOUNTING_HOME_TEST_QML_FILES}
)

target_link_libraries(StuKitAccountingHomeQmlIntegrationTest
    PRIVATE
        Qt6::Quick
        Qt6::Test
)

add_test(
    NAME app.accounting_home_qml
    COMMAND StuKitAccountingHomeQmlIntegrationTest
)

set_tests_properties(app.accounting_home_qml PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QPA_FONTDIR=C:/Windows/Fonts;QT_QUICK_CONTROLS_STYLE=Basic"
)
