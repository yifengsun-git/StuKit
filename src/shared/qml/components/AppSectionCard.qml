import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: root

    default property alias contentData: body.data
    property string title: ""
    property string description: ""
    property color accentColor: "transparent"
    property int contentPadding: AppTheme.spacingLarge
    readonly property real visibleContentHeight: {
        let height = 0
        let visibleItemCount = 0
        for (const child of body.children) {
            if (!child.visible)
                continue

            let itemHeight = child.implicitHeight
            if (child.Layout.preferredHeight >= 0) {
                itemHeight = Math.max(
                    itemHeight, child.Layout.preferredHeight)
            }
            if (child.Layout.minimumHeight >= 0) {
                itemHeight = Math.max(
                    itemHeight, child.Layout.minimumHeight)
            }
            height += itemHeight
                + Math.max(0, child.Layout.topMargin)
                + Math.max(0, child.Layout.bottomMargin)
            visibleItemCount += 1
        }
        return height + Math.max(0, visibleItemCount - 1) * body.spacing
    }

    padding: contentPadding
    // Include attached layout hints. TextArea and SplitView commonly express
    // their intended height through Layout.preferredHeight instead of
    // implicitHeight, while expandable panels also rely on minimumHeight.
    implicitHeight: visibleContentHeight + topPadding + bottomPadding

    contentItem: ColumnLayout {
        id: body

        spacing: AppTheme.spacingSmall

        Label {
            Layout.fillWidth: true
            visible: root.title.length > 0
            text: root.title
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeSubtitle
            font.bold: true
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true
            visible: root.description.length > 0
            text: root.description
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody
            wrapMode: Text.WordWrap
        }
    }

    background: Rectangle {
        radius: AppTheme.radiusLarge
        color: AppTheme.surfaceRaisedColor
        border.width: 1
        border.color: root.accentColor.a > 0
            ? root.accentColor : AppTheme.borderColor
    }
}
