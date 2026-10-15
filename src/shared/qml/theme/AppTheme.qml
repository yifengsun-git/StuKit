pragma Singleton

import QtQuick

QtObject {
    property bool darkMode: false

    readonly property color backgroundColor: darkMode ? "#111827" : "#F3F4F6"
    readonly property color surfaceColor: darkMode ? "#1F2937" : "#FFFFFF"
    readonly property color surfaceRaisedColor: darkMode ? "#273449" : "#FFFFFF"
    readonly property color surfaceVariantColor: darkMode ? "#374151" : "#E5E7EB"
    readonly property color surfaceHoverColor: darkMode ? "#334155" : "#F8FAFC"
    readonly property color selectionColor: darkMode ? "#1E3A5F" : "#DBEAFE"
    readonly property color primaryColor: darkMode ? "#60A5FA" : "#2563EB"
    readonly property color primaryHoverColor: darkMode ? "#93C5FD" : "#1D4ED8"
    readonly property color primaryPressedColor: darkMode ? "#3B82F6" : "#1D4ED8"
    readonly property color textPrimaryColor: darkMode ? "#F9FAFB" : "#111827"
    readonly property color textSecondaryColor: darkMode ? "#D1D5DB" : "#4B5563"
    readonly property color borderColor: darkMode ? "#4B5563" : "#D1D5DB"
    readonly property color borderStrongColor: darkMode ? "#64748B" : "#94A3B8"
    readonly property color textOnPrimaryColor: "#FFFFFF"
    readonly property color errorColor: darkMode ? "#FCA5A5" : "#DC2626"
    readonly property color successColor: darkMode ? "#6EE7B7" : "#059669"
    readonly property color warningColor: darkMode ? "#FCD34D" : "#D97706"

    readonly property int spacingExtraSmall: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int spacingExtraLarge: 32

    readonly property int radiusSmall: 6
    readonly property int radiusMedium: 10
    readonly property int radiusLarge: 16

    readonly property int fontSizeBody: 14
    readonly property int fontSizeCaption: 12
    readonly property int fontSizeSubtitle: 16
    readonly property int fontSizeTitle: 20
    readonly property int fontSizeHeading: 24
    readonly property int fontSizeDisplay: 32

    readonly property int controlHeight: 44
    readonly property int touchTargetHeight: 48
    readonly property int navigationRailWidth: 224
    readonly property int contentMaxWidth: 1180

    readonly property int breakpointCompact: 720
    readonly property int breakpointWide: 1100

    readonly property int animationFast: 100
    readonly property int animationNormal: 180

    function pagePadding(availableWidth) {
        return availableWidth < breakpointCompact
            ? spacingMedium : spacingLarge
    }
}
