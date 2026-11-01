pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as PlatformDialogs
import QtQuick.Layouts

Page {
    id: root
    objectName: "appearancePage"

    required property var viewModel

    readonly property bool compactLayout:
        width < AppTheme.breakpointCompact

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight
            + AppTheme.spacingExtraLarge * 2

        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: contentColumn

            width: Math.min(
                parent.width - AppTheme.spacingLarge * 2,
                AppTheme.contentMaxWidth)
            anchors.horizontalCenter: parent.horizontalCenter
            y: AppTheme.spacingExtraLarge
            spacing: AppTheme.spacingLarge

            Label {
                Layout.fillWidth: true
                text: qsTr("外观与主题")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeHeading
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("主题会统一改变颜色、卡片、按钮和圆角，不会修改你的账单、课表或待办数据。")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeBody
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall

                Label {
                    Layout.fillWidth: true
                    text: qsTr("共 %1 套可用主题")
                        .arg(root.viewModel ? root.viewModel.themes.length : 0)
                    color: AppTheme.textSecondaryColor
                }

                AppButton {
                    objectName: "appearanceImportThemeButton"
                    text: qsTr("导入主题")
                    onClicked: themeFileDialog.open()
                }
            }

            Label {
                Layout.fillWidth: true
                visible: root.viewModel && root.viewModel.hasError
                text: root.viewModel ? root.viewModel.errorMessage : ""
                color: AppTheme.errorColor
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                visible: root.viewModel
                    && root.viewModel.statusMessage.length > 0
                text: root.viewModel ? root.viewModel.statusMessage : ""
                color: AppTheme.successColor
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: currentThemeContent.implicitHeight
                    + AppTheme.spacingLarge * 2
                radius: AppTheme.radiusLarge
                color: AppTheme.surfaceRaisedColor
                border.color: AppTheme.primaryColor
                border.width: 1

                RowLayout {
                    id: currentThemeContent

                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingLarge
                    spacing: AppTheme.spacingMedium

                    Rectangle {
                        Layout.preferredWidth: 12
                        Layout.fillHeight: true
                        radius: width / 2
                        color: AppTheme.primaryColor
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingExtraSmall

                        Label {
                            text: qsTr("当前主题")
                            color: AppTheme.textSecondaryColor
                            font.pixelSize: AppTheme.fontSizeCaption
                        }

                        Label {
                            text: AppTheme.themeDisplayName
                            color: AppTheme.textPrimaryColor
                            font.pixelSize: AppTheme.fontSizeTitle
                            font.bold: true
                        }
                    }

                    AppButton {
                        text: qsTr("组件预览")
                        onClicked: {
                            themeDetailDialog.definition
                                = root.viewModel.currentTheme
                            themeDetailDialog.open()
                        }
                    }

                    AppButton {
                        objectName: "appearanceCurrentThemeDiagnosticsButton"
                        text: qsTr("主题诊断")
                        onClicked: themeDiagnosticsDialog.showFor(
                            root.viewModel.currentTheme)
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.compactLayout ? 1 : 2
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                Repeater {
                    objectName: "appearanceThemeRepeater"
                    model: root.viewModel ? root.viewModel.themes : []

                    delegate: Rectangle {
                        id: themeCard

                        required property var modelData
                        readonly property bool selected:
                            root.viewModel
                            && root.viewModel.themeId
                                === modelData.themeId

                        objectName: "appearanceThemeCard_"
                            + modelData.themeId
                        Layout.fillWidth: true
                        Layout.preferredHeight:
                            themeCard.modelData.builtIn ? 286 : 350
                        radius: AppTheme.radiusLarge
                        color: AppTheme.surfaceColor
                        border.width: selected ? 2 : 1
                        border.color: selected
                            ? AppTheme.primaryColor : AppTheme.borderColor

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: AppTheme.spacingLarge
                            spacing: AppTheme.spacingMedium

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 72
                                radius: AppTheme.radiusMedium
                                color: themeCard.modelData.backgroundColor
                                border.color: AppTheme.borderColor

                                Image {
                                    anchors.fill: parent
                                    visible: source.toString().length > 0
                                    source: themeCard.modelData.previewUrl || ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    cache: false
                                    clip: true
                                }

                                Row {
                                    anchors.centerIn: parent
                                    spacing: AppTheme.spacingSmall
                                    visible: !(themeCard.modelData.previewUrl || "")

                                    Rectangle {
                                        width: 42
                                        height: 42
                                        radius: AppTheme.radiusSmall
                                        color: themeCard.modelData.surfaceColor
                                    }

                                    Rectangle {
                                        width: 42
                                        height: 42
                                        radius: AppTheme.radiusSmall
                                        color: themeCard.modelData.primaryColor
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    Layout.fillWidth: true
                                    text: themeCard.modelData.name
                                    color: AppTheme.textPrimaryColor
                                    font.pixelSize: AppTheme.fontSizeSubtitle
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    visible: themeCard.selected
                                    text: qsTr("使用中")
                                    color: AppTheme.primaryColor
                                    font.bold: true
                                }
                            }

                            Label {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                text: themeCard.modelData.description
                                color: AppTheme.textSecondaryColor
                                wrapMode: Text.WordWrap
                                verticalAlignment: Text.AlignTop
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("作者：%1 · 版本：%2")
                                    .arg(themeCard.modelData.author)
                                    .arg(themeCard.modelData.version)
                                color: AppTheme.textSecondaryColor
                                font.pixelSize: AppTheme.fontSizeCaption
                                elide: Text.ElideRight
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: AppTheme.spacingSmall

                                AppButton {
                                    objectName: "appearanceSelectTheme_"
                                        + themeCard.modelData.themeId
                                    Layout.fillWidth: true
                                    text: themeCard.selected
                                        ? qsTr("当前主题") : qsTr("使用此主题")
                                    highlighted: themeCard.selected
                                    enabled: !themeCard.selected
                                    onClicked: root.viewModel.selectTheme(
                                        themeCard.modelData.themeId)
                                }

                                AppButton {
                                    objectName: "appearanceThemeDetails_"
                                        + themeCard.modelData.themeId
                                    text: qsTr("详情")
                                    onClicked: {
                                        themeDetailDialog.definition
                                            = themeCard.modelData
                                        themeDetailDialog.open()
                                    }
                                }

                                AppButton {
                                    objectName: "appearanceThemeDiagnostics_"
                                        + themeCard.modelData.themeId
                                    text: qsTr("诊断")
                                    onClicked: themeDiagnosticsDialog.showFor(
                                        themeCard.modelData)
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                visible: !themeCard.modelData.builtIn
                                spacing: AppTheme.spacingSmall

                                AppButton {
                                    objectName: "appearanceExportTheme_"
                                        + themeCard.modelData.themeId
                                    Layout.fillWidth: true
                                    text: qsTr("导出备份")
                                    onClicked: {
                                        exportThemeFileDialog.themeId
                                            = themeCard.modelData.themeId
                                        exportThemeFileDialog.themeName
                                            = themeCard.modelData.name
                                        exportThemeFileDialog.open()
                                    }
                                }

                                AppButton {
                                    objectName: "appearanceRemoveTheme_"
                                        + themeCard.modelData.themeId
                                    text: qsTr("删除")
                                    onClicked: {
                                        removeThemeDialog.pendingThemeId
                                            = themeCard.modelData.themeId
                                        removeThemeDialog.pendingThemeName
                                            = themeCard.modelData.name
                                        removeThemeDialog.open()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("第三方 .stukit-theme 文件只包含经过验证的设计令牌，不会执行 QML 或 JavaScript。")
                    + qsTr(" 资源包可以包含预览图和受限图片资源。")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeCaption
                wrapMode: Text.WordWrap
            }
        }
    }

    PlatformDialogs.FileDialog {
        id: themeFileDialog
        title: qsTr("导入 StuKit 主题")
        fileMode: PlatformDialogs.FileDialog.OpenFile
        nameFilters: [
            qsTr("StuKit 主题 (*.stukit-theme *.json)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            if (root.viewModel.inspectTheme(selectedFile))
                importPreviewDialog.open()
        }
    }

    PlatformDialogs.FileDialog {
        id: exportThemeFileDialog

        property string themeId: ""
        property string themeName: ""

        title: qsTr("导出主题“%1”").arg(themeName)
        fileMode: PlatformDialogs.FileDialog.SaveFile
        nameFilters: [qsTr("StuKit 主题 (*.stukit-theme)")]
        onAccepted: {
            root.viewModel.exportTheme(themeId, selectedFile)
            themeId = ""
            themeName = ""
        }
        onRejected: {
            themeId = ""
            themeName = ""
        }
    }

    AppDialog {
        id: importPreviewDialog

        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 700
        title: root.viewModel && root.viewModel.pendingThemeReplacesExisting
            ? qsTr("确认更新主题") : qsTr("确认安装主题")
        standardButtons: Dialog.Yes | Dialog.Cancel
        enterActionEnabled: true
        onEnterActionRequested: accept()
        onAccepted: root.viewModel.installPendingTheme()
        onRejected: root.viewModel.cancelThemeImport()

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: root.viewModel && root.viewModel.pendingThemeReplacesExisting
                    ? qsTr("已安装同一 themeId 的主题。继续后会安全更新它，并立即启用新版本。")
                    : qsTr("主题已通过格式和安全校验。确认后才会写入主题目录并启用。")
                color: root.viewModel && root.viewModel.pendingThemeReplacesExisting
                    ? AppTheme.warningColor : AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: root.viewModel && root.viewModel.hasPendingTheme
                    ? qsTr("%1 · 作者 %2 · 版本 %3")
                        .arg(root.viewModel.pendingTheme.name)
                        .arg(root.viewModel.pendingTheme.author)
                        .arg(root.viewModel.pendingTheme.version)
                    : ""
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: root.viewModel && root.viewModel.hasPendingTheme
                    ? root.viewModel.pendingTheme.description : ""
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            ThemePreviewPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 338
                definition: root.viewModel && root.viewModel.hasPendingTheme
                    ? root.viewModel.pendingTheme : ({})
            }
        }
    }

    AppDialog {
        id: themeDetailDialog

        property var definition: ({})

        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 700
        title: qsTr("主题详情")
        standardButtons: Dialog.Close

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("%1 · 作者 %2 · 版本 %3")
                    .arg(themeDetailDialog.definition.name || "")
                    .arg(themeDetailDialog.definition.author || "")
                    .arg(themeDetailDialog.definition.version || "")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: themeDetailDialog.definition.description || ""
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            ThemePreviewPanel {
                Layout.fillWidth: true
                Layout.preferredHeight: 338
                definition: themeDetailDialog.definition
            }
        }
    }

    AppDialog {
        id: themeDiagnosticsDialog

        property var definition: ({})
        property var diagnostics: []

        function showFor(themeDefinition) {
            definition = themeDefinition
            diagnostics = root.viewModel.themeDiagnostics(
                themeDefinition.themeId)
            open()
        }

        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 680
        title: qsTr("主题开发者诊断")
        standardButtons: Dialog.Close

        ColumnLayout {
            Layout.fillWidth: true
            spacing: AppTheme.spacingMedium

            Label {
                Layout.fillWidth: true
                text: qsTr("%1 · %2 项检查")
                    .arg(themeDiagnosticsDialog.definition.name || "")
                    .arg(themeDiagnosticsDialog.diagnostics.length)
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeSubtitle
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("对比度未达建议值不会阻止主题使用；资源缺失则表示主题包需要修复。")
                color: AppTheme.textSecondaryColor
                wrapMode: Text.WordWrap
            }

            Repeater {
                objectName: "appearanceThemeDiagnosticsRepeater"
                model: themeDiagnosticsDialog.diagnostics

                delegate: Rectangle {
                    id: diagnosticRow

                    required property var modelData

                    Layout.fillWidth: true
                    implicitHeight: diagnosticContent.implicitHeight
                        + AppTheme.spacingMedium * 2
                    radius: AppTheme.radiusMedium
                    color: AppTheme.surfaceColor
                    border.color: modelData.passed
                        ? AppTheme.borderColor
                        : (modelData.severity === "error"
                            ? AppTheme.errorColor : AppTheme.warningColor)

                    RowLayout {
                        id: diagnosticContent

                        anchors.fill: parent
                        anchors.margins: AppTheme.spacingMedium
                        spacing: AppTheme.spacingSmall

                        Label {
                            text: diagnosticRow.modelData.passed ? "✓" : "!"
                            color: diagnosticRow.modelData.passed
                                ? AppTheme.successColor
                                : (diagnosticRow.modelData.severity === "error"
                                    ? AppTheme.errorColor : AppTheme.warningColor)
                            font.bold: true
                            font.pixelSize: AppTheme.fontSizeTitle
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: AppTheme.spacingExtraSmall

                            Label {
                                Layout.fillWidth: true
                                text: diagnosticRow.modelData.label
                                color: AppTheme.textPrimaryColor
                                font.bold: true
                            }

                            Label {
                                Layout.fillWidth: true
                                text: diagnosticRow.modelData.detail
                                color: AppTheme.textSecondaryColor
                                font.pixelSize: AppTheme.fontSizeCaption
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
        }
    }

    AppDialog {
        id: removeThemeDialog

        property string pendingThemeId: ""
        property string pendingThemeName: ""

        anchors.centerIn: parent
        width: Math.min(root.width - AppTheme.spacingLarge * 2, 420)
        modal: true
        title: qsTr("删除第三方主题")
        standardButtons: Dialog.Yes | Dialog.Cancel
        onAccepted: {
            root.viewModel.removeTheme(pendingThemeId)
            pendingThemeId = ""
            pendingThemeName = ""
        }
        onRejected: {
            pendingThemeId = ""
            pendingThemeName = ""
        }

        Label {
            width: parent.width
            text: qsTr("确定删除主题“%1”吗？如果正在使用，将自动切换回默认主题。")
                .arg(removeThemeDialog.pendingThemeName)
            color: AppTheme.textPrimaryColor
            wrapMode: Text.WordWrap
        }
    }
}
