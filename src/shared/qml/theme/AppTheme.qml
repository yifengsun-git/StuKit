pragma Singleton

import QtQuick

QtObject {
    property bool darkMode: false

    readonly property color backgroundColor: darkMode ? "#111827" : "#F3F4F6"
    readonly property color surfaceColor: darkMode ? "#1F2937" : "#FFFFFF"
    readonly property color surfaceVariantColor: darkMode ? "#374151" : "#E5E7EB"
    readonly property color primaryColor: darkMode ? "#60A5FA" : "#2563EB"
    readonly property color textPrimaryColor: darkMode ? "#F9FAFB" : "#111827"
    readonly property color textSecondaryColor: darkMode ? "#D1D5DB" : "#4B5563"
    readonly property color borderColor: darkMode ? "#4B5563" : "#D1D5DB"
    readonly property color textOnPrimaryColor: "#FFFFFF"
    readonly property color errorColor: darkMode ? "#FCA5A5" : "#DC2626"

    readonly property int spacingExtraSmall: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int spacingExtraLarge: 32

    readonly property int radiusSmall: 6
    readonly property int radiusMedium: 10
    readonly property int radiusLarge: 16

    readonly property int fontSizeBody: 14
    readonly property int fontSizeSubtitle: 16
    readonly property int fontSizeHeading: 24
    readonly property int fontSizeDisplay: 28

    readonly property int controlHeight: 44
}
