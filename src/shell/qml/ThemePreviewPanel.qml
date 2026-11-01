pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var definition

    function token(name, fallbackValue) {
        if (!definition || definition[name] === undefined
                || definition[name] === null)
            return fallbackValue
        return definition[name]
    }

    readonly property color previewBackground:
        token("backgroundColor", AppTheme.backgroundColor)
    readonly property color previewSurface:
        token("surfaceColor", AppTheme.surfaceColor)
    readonly property color previewSurfaceVariant:
        token("surfaceVariantColor", AppTheme.surfaceVariantColor)
    readonly property color previewPrimary:
        token("primaryColor", AppTheme.primaryColor)
    readonly property color previewText:
        token("textPrimaryColor", AppTheme.textPrimaryColor)
    readonly property color previewSecondaryText:
        token("textSecondaryColor", AppTheme.textSecondaryColor)
    readonly property color previewBorder:
        token("borderColor", AppTheme.borderColor)
    readonly property color previewOnPrimary:
        token("textOnPrimaryColor", AppTheme.textOnPrimaryColor)
    readonly property real previewRadius:
        token("radiusMedium", AppTheme.radiusMedium)

    objectName: "themePreviewPanel"
    implicitWidth: 620
    implicitHeight: 338
    radius: token("radiusLarge", AppTheme.radiusLarge)
    color: previewBackground
    border.color: previewBorder
    clip: true

    Image {
        anchors.fill: parent
        source: root.token("backgroundImageUrl", "")
        visible: source.toString().length > 0 && opacity > 0
        opacity: root.token("backgroundImageOpacity", 0)
        fillMode: root.token("backgroundImageFillMode", "cover") === "contain"
            ? Image.PreserveAspectFit
            : (root.token("backgroundImageFillMode", "cover") === "tile"
                ? Image.Tile : Image.PreserveAspectCrop)
        asynchronous: true
        cache: false
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 142
            Layout.fillHeight: true
            radius: root.previewRadius
            color: root.previewSurface
            border.color: root.previewBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label {
                    visible: !brandPreview.visible
                    text: qsTr("StuKit")
                    color: root.previewPrimary
                    font.pixelSize: 20
                    font.bold: true
                }

                Image {
                    id: brandPreview

                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 42 : 0
                    visible: source.toString().length > 0
                    source: root.token("navigationLogoUrl", "")
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: false
                }

                Repeater {
                    model: [qsTr("首页"), qsTr("记账本"), qsTr("课程表")]

                    delegate: Rectangle {
                        required property string modelData
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        radius: root.token("radiusSmall", 6)
                        color: index === 1
                            ? root.token("selectionColor", root.previewSurfaceVariant)
                            : "transparent"

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            text: parent.modelData
                            color: index === 1
                                ? root.previewPrimary : root.previewSecondaryText
                            font.bold: index === 1
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Image {
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 96 : 0
                    visible: source.toString().length > 0
                    source: root.token("previewUrl", "")
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: false
                    clip: true
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("主题组件预览")
                    color: root.previewSecondaryText
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Label {
                text: qsTr("本月概览")
                color: root.previewText
                font.pixelSize: 20
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Repeater {
                    model: [
                        { label: qsTr("收入"), value: "¥ 3,200", color: root.token("successColor", "#059669") },
                        { label: qsTr("支出"), value: "¥ 1,460", color: root.token("errorColor", "#DC2626") }
                    ]

                    delegate: Rectangle {
                        required property var modelData

                        Layout.fillWidth: true
                        Layout.preferredHeight: 76
                        radius: root.previewRadius
                        color: root.previewSurface
                        border.color: root.previewBorder

                        Column {
                            anchors.centerIn: parent
                            spacing: 4

                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: parent.parent.modelData.label
                                color: root.previewSecondaryText
                                font.pixelSize: 11
                            }
                            Label {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: parent.parent.modelData.value
                                color: parent.parent.modelData.color
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                radius: root.previewRadius
                color: root.previewSurface
                border.color: root.previewBorder

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("午餐 · 餐饮")
                    color: root.previewText
                }

                Label {
                    anchors.right: parent.right
                    anchors.rightMargin: 14
                    anchors.verticalCenter: parent.verticalCenter
                    text: "- ¥ 28.00"
                    color: root.token("errorColor", "#DC2626")
                    font.bold: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                radius: root.token("radiusSmall", 6)
                color: root.previewSurface
                border.color: root.previewBorder

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("输入框示例")
                    color: root.previewSecondaryText
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: root.previewRadius
                    color: root.previewPrimary

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("主要按钮")
                        color: root.previewOnPrimary
                        font.bold: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    radius: root.previewRadius
                    color: root.previewSurfaceVariant
                    border.color: root.previewBorder

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("次要按钮")
                        color: root.previewText
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
