pragma Singleton

import QtQuick

QtObject {
    property string themeId: "default"
    property string themeDisplayName: qsTr("StuKit 默认")
    property bool darkMode: false

    property color backgroundColor: "#F3F4F6"
    property color surfaceColor: "#FFFFFF"
    property color surfaceRaisedColor: "#FFFFFF"
    property color surfaceVariantColor: "#E5E7EB"
    property color surfaceHoverColor: "#F8FAFC"
    property color selectionColor: "#DBEAFE"
    property color primaryColor: "#2563EB"
    property color primaryHoverColor: "#1D4ED8"
    property color primaryPressedColor: "#1D4ED8"
    property color textPrimaryColor: "#111827"
    property color textSecondaryColor: "#4B5563"
    property color borderColor: "#D1D5DB"
    property color borderStrongColor: "#94A3B8"
    property color textOnPrimaryColor: "#FFFFFF"
    property color errorColor: "#DC2626"
    property color successColor: "#059669"
    property color warningColor: "#D97706"

    property url backgroundImageUrl: ""
    property real backgroundImageOpacity: 0
    property string backgroundImageFillMode: "cover"
    property url navigationLogoUrl: ""
    property var navigationIconUrls: ({})

    readonly property int spacingExtraSmall: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int spacingExtraLarge: 32

    property int radiusSmall: 6
    property int radiusMedium: 10
    property int radiusLarge: 16

    readonly property int fontSizeBody: 14
    readonly property int fontSizeCaption: 12
    readonly property int fontSizeSubtitle: 16
    readonly property int fontSizeTitle: 20
    readonly property int fontSizeHeading: 24
    readonly property int fontSizeDisplay: 32

    readonly property int controlHeight: 44
    readonly property int touchTargetHeight: 48
    property int navigationRailWidth: 224
    readonly property int contentMaxWidth: 1180

    readonly property int breakpointCompact: 720
    readonly property int breakpointWide: 1100

    readonly property int animationFast: 100
    readonly property int animationNormal: 180

    function applyTheme(definition) {
        if (!definition || !definition.themeId
                || !definition.primaryColor
                || !definition.backgroundColor) {
            return false
        }

        themeId = definition.themeId
        themeDisplayName = definition.name
        darkMode = definition.dark
        backgroundColor = definition.backgroundColor
        surfaceColor = definition.surfaceColor
        surfaceRaisedColor = definition.surfaceRaisedColor
        surfaceVariantColor = definition.surfaceVariantColor
        surfaceHoverColor = definition.surfaceHoverColor
        selectionColor = definition.selectionColor
        primaryColor = definition.primaryColor
        primaryHoverColor = definition.primaryHoverColor
        primaryPressedColor = definition.primaryPressedColor
        textPrimaryColor = definition.textPrimaryColor
        textSecondaryColor = definition.textSecondaryColor
        borderColor = definition.borderColor
        borderStrongColor = definition.borderStrongColor
        textOnPrimaryColor = definition.textOnPrimaryColor
        errorColor = definition.errorColor
        successColor = definition.successColor
        warningColor = definition.warningColor
        backgroundImageUrl = definition.backgroundImageUrl || ""
        backgroundImageOpacity = definition.backgroundImageUrl
            ? (definition.backgroundImageOpacity || 0) : 0
        backgroundImageFillMode = definition.backgroundImageFillMode || "cover"
        navigationLogoUrl = definition.navigationLogoUrl || ""
        navigationIconUrls = definition.navigationIconUrls || ({})
        radiusSmall = definition.radiusSmall
        radiusMedium = definition.radiusMedium
        radiusLarge = definition.radiusLarge
        navigationRailWidth = definition.navigationRailWidth
        return true
    }

    function navigationIcon(slot) {
        if (!navigationIconUrls || navigationIconUrls[slot] === undefined)
            return ""
        return navigationIconUrls[slot]
    }

    function pagePadding(availableWidth) {
        return availableWidth < breakpointCompact
            ? spacingMedium : spacingLarge
    }
}
