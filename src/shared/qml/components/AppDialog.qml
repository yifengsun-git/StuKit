import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: control

    default property alias dialogContentData: contentLayout.data
    property real viewportWidth: parent ? parent.width : 0
    property real viewportHeight: parent ? parent.height : 0
    property real preferredWidth: 520
    readonly property real maximumDialogHeight: Math.max(
        0, viewportHeight - AppTheme.spacingMedium * 2)
    readonly property bool contentScrollable:
        contentScroll.contentHeight > contentScroll.height + 0.5

    function handleBackRequest() {
        if (!opened)
            return false
        reject()
        return true
    }

    function ensureActiveFocusVisible() {
        const dialogWindow = contentScroll.Window.window
        const flickable = contentScroll.contentItem
        if (!opened || !dialogWindow || !flickable
                || contentScroll.availableHeight <= 0)
            return

        const focusItem = dialogWindow.activeFocusItem
        if (!focusItem)
            return

        let ancestor = focusItem
        let belongsToContent = false
        while (ancestor) {
            if (ancestor === contentLayout) {
                belongsToContent = true
                break
            }
            ancestor = ancestor.parent
        }
        if (!belongsToContent)
            return

        const position = focusItem.mapToItem(contentLayout, 0, 0)
        const margin = AppTheme.spacingSmall
        const visibleTop = flickable.contentY
        const visibleBottom = visibleTop + contentScroll.availableHeight
        const itemBottom = position.y + focusItem.height

        if (position.y - margin < visibleTop) {
            flickable.contentY = Math.max(0, position.y - margin)
        } else if (itemBottom + margin > visibleBottom) {
            const maximumY = Math.max(
                0, flickable.contentHeight - contentScroll.availableHeight)
            flickable.contentY = Math.min(
                maximumY,
                itemBottom + margin - contentScroll.availableHeight)
        }
    }

    width: Math.max(0, Math.min(
        viewportWidth - AppTheme.spacingMedium * 2,
        preferredWidth))
    height: maximumDialogHeight > 0
        ? Math.min(implicitHeight, maximumDialogHeight)
        : implicitHeight
    anchors.centerIn: parent
    modal: true
    focus: true
    padding: AppTheme.spacingLarge

    onOpened: Qt.callLater(ensureActiveFocusVisible)

    contentItem: ScrollView {
        id: contentScroll

        clip: true
        contentWidth: availableWidth
        implicitWidth: contentLayout.implicitWidth
        implicitHeight: contentLayout.implicitHeight
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape)
                event.accepted = control.handleBackRequest()
        }

        ColumnLayout {
            id: contentLayout

            width: contentScroll.availableWidth
            spacing: 0
        }
    }

    Connections {
        target: contentScroll.Window.window

        function onActiveFocusItemChanged() {
            Qt.callLater(control.ensureActiveFocusVisible)
        }
    }

    Connections {
        target: Qt.inputMethod

        function onVisibleChanged() {
            Qt.callLater(control.ensureActiveFocusVisible)
        }

        function onKeyboardRectangleChanged() {
            Qt.callLater(control.ensureActiveFocusVisible)
        }
    }

    background: Rectangle {
        radius: AppTheme.radiusLarge
        color: AppTheme.surfaceRaisedColor
        border.width: 1
        border.color: AppTheme.borderColor
    }
}
