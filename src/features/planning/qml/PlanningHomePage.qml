import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel

    property int pendingImportKind: 0
    property string pendingImportTitle: ""
    property string pendingAnalysisInstructions: ""
    property bool factsExpanded: false
    readonly property int pagePadding: AppTheme.pagePadding(width)
    readonly property bool narrowLayout: width < 900

    function resetProjectForm() {
        projectTitleField.clear()
        projectObjectiveField.clear()
        projectConstraintsField.clear()
    }

    function openProjectEditor() {
        editProjectTitleField.text = root.viewModel.selectedProjectTitle
        editProjectObjectiveField.text
            = root.viewModel.selectedProjectObjective
        editProjectConstraintsField.text
            = root.viewModel.selectedProjectConstraints
        editProjectDialog.open()
        editProjectTitleField.forceActiveFocus()
    }

    function resetPasteForm() {
        sourceTitleField.clear()
        sourceContentArea.clear()
        sourceKindCombo.currentIndex = 0
        sourceFormatCombo.currentIndex = 0
    }

    function openSourceDetails(sourceId) {
        if (!root.viewModel.selectSource(sourceId))
            return
        sourceDetailTitleField.text = root.viewModel.selectedSourceTitle
        sourceDetailKindCombo.currentIndex
            = root.viewModel.selectedSourceKindIndex
        sourceDetailFormatCombo.currentIndex
            = root.viewModel.selectedSourceFormatIndex
        sourceDetailContentArea.text = root.viewModel.selectedSourceContent
        sourceDetailTabs.currentIndex = 0
        sourceDetailsDialog.open()
    }

    function safeMarkdownPreview(value) {
        return String(value).replace(
            /!\[([^\]]*)\]\([^)]+\)/g,
            "[$1（预览已隐藏外部图片）]")
    }

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    Connections {
        target: root.viewModel

        function onGeneratedPlanReady(markdownContent) {
            planEditor.text = markdownContent
        }

        function onPlanChanged() {
            if (!planEditor.activeFocus)
                planEditor.text = root.viewModel.planMarkdown
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            x: Math.max(root.pagePadding,
                (scrollView.availableWidth - width) / 2)
            width: Math.max(0, Math.min(
                scrollView.availableWidth - root.pagePadding * 2,
                AppTheme.contentMaxWidth))
            spacing: AppTheme.spacingMedium

            Item { Layout.preferredHeight: AppTheme.spacingSmall }

            RowLayout {
                Layout.fillWidth: true
                spacing: AppTheme.spacingSmall

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingExtraSmall

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("规划助手")
                        color: AppTheme.textPrimaryColor
                        font.pixelSize: root.narrowLayout
                            ? AppTheme.fontSizeHeading
                            : AppTheme.fontSizeDisplay
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("先建立目标和可信资料，再让 AI 参与分析。")
                        color: AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }
                }

                AppButton {
                    objectName: "planningNewProjectButton"
                    text: root.narrowLayout ? qsTr("新建") : qsTr("新建规划")
                    highlighted: true
                    onClicked: {
                        root.viewModel.clearMessages()
                        root.resetProjectForm()
                        projectDialog.open()
                        projectTitleField.forceActiveFocus()
                    }
                }
            }

            AppSectionCard {
                visible: root.viewModel.errorMessage.length > 0
                Layout.fillWidth: true
                title: qsTr("操作失败")
                description: root.viewModel.errorMessage
                accentColor: AppTheme.errorColor
            }

            AppSectionCard {
                visible: root.viewModel.statusMessage.length > 0
                Layout.fillWidth: true
                title: qsTr("已完成")
                description: root.viewModel.statusMessage
                accentColor: AppTheme.successColor
            }

            AppSectionCard {
                visible: !root.viewModel.hasProjects
                Layout.fillWidth: true
                title: qsTr("从一个规划目标开始")
                description: qsTr("例如“完成本科培养要求并准备软件开发实习”。项目建立后，可以粘贴培养方案，或导入 UTF-8 编码的文本、Markdown 和 CSV 文件。")
                accentColor: AppTheme.primaryColor

                AppButton {
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("创建第一个规划")
                    highlighted: true
                    onClicked: {
                        root.resetProjectForm()
                        projectDialog.open()
                    }
                }
            }

            GridLayout {
                visible: root.viewModel.hasProjects
                Layout.fillWidth: true
                columns: root.narrowLayout ? 1 : 2
                columnSpacing: AppTheme.spacingMedium
                rowSpacing: AppTheme.spacingMedium

                AppSectionCard {
                    Layout.fillWidth: true
                    Layout.columnSpan: root.narrowLayout ? 1 : 2
                    title: qsTr("事实与校验")
                    description: root.viewModel.extractedFactCount > 0
                        ? qsTr("从原文确定性提取 %1 条事实，发现 %2 条需要核对的信息。引用可定位到资料和行号。")
                            .arg(root.viewModel.extractedFactCount)
                            .arg(root.viewModel.validationIssueCount)
                        : qsTr("加入资料后，会在本地提取日期、截止时间、学分、时长、金额和明确要求。")
                    accentColor: root.viewModel.validationIssueCount > 0
                        ? AppTheme.warningColor : AppTheme.successColor

                    ColumnLayout {
                        visible: root.viewModel.validationIssues.length > 0
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingExtraSmall

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("需要人工核对")
                            color: AppTheme.textPrimaryColor
                            font.bold: true
                        }
                        Repeater {
                            model: root.viewModel.validationIssues

                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: issueLabel.implicitHeight
                                    + AppTheme.spacingSmall * 2
                                radius: AppTheme.radiusSmall
                                color: modelData.severity === qsTr("错误")
                                    ? AppTheme.errorColor
                                    : AppTheme.selectionColor
                                opacity: 0.9

                                Label {
                                    id: issueLabel
                                    anchors.fill: parent
                                    anchors.margins: AppTheme.spacingSmall
                                    text: (modelData.citation.length > 0
                                        ? "[" + modelData.citation + "] " : "")
                                        + modelData.message
                                    color: modelData.severity === qsTr("错误")
                                        ? AppTheme.textOnPrimaryColor
                                        : AppTheme.textPrimaryColor
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    Label {
                        visible: root.viewModel.extractedFacts.length > 0
                        Layout.fillWidth: true
                        Layout.topMargin: AppTheme.spacingSmall
                        text: qsTr("已提取事实")
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }
                    Repeater {
                        model: root.viewModel.extractedFacts

                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            visible: root.factsExpanded || index < 6
                            Layout.fillWidth: true
                            implicitHeight: visible
                                ? factContent.implicitHeight
                                    + AppTheme.spacingSmall * 2
                                : 0
                            radius: AppTheme.radiusSmall
                            color: AppTheme.surfaceColor
                            border.color: AppTheme.borderColor

                            ColumnLayout {
                                id: factContent
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingSmall
                                spacing: AppTheme.spacingExtraSmall

                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        text: modelData.kind
                                        color: AppTheme.primaryColor
                                        font.bold: true
                                    }
                                    Label {
                                        Layout.fillWidth: true
                                        text: modelData.value
                                        color: AppTheme.textPrimaryColor
                                        font.bold: true
                                        wrapMode: Text.WordWrap
                                    }
                                    Label {
                                        text: "[" + modelData.citation + "]"
                                        color: AppTheme.textSecondaryColor
                                        font.pixelSize: AppTheme.fontSizeCaption
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: modelData.excerpt
                                    color: AppTheme.textSecondaryColor
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                    AppButton {
                        visible: root.viewModel.extractedFacts.length > 6
                        text: root.factsExpanded
                            ? qsTr("收起事实")
                            : qsTr("展开前 %1 条").arg(
                                root.viewModel.extractedFacts.length)
                        onClicked: root.factsExpanded = !root.factsExpanded
                    }
                    Label {
                        visible: root.viewModel.extractedFactCount > 30
                        Layout.fillWidth: true
                        text: qsTr("为避免页面过长，只显示前 30 条；全部事实仍会参与本次分析。")
                        color: AppTheme.textSecondaryColor
                        font.pixelSize: AppTheme.fontSizeCaption
                        wrapMode: Text.WordWrap
                    }
                }

                AppSectionCard {
                    visible: root.viewModel.opportunityCount > 0
                    Layout.fillWidth: true
                    Layout.columnSpan: root.narrowLayout ? 1 : 2
                    title: qsTr("机会优先级")
                    description: qsTr("评分由目标相关度 40 分、截止紧迫度 25 分、投入可行性 25 分和资料完整度 10 分组成。它是决策辅助，不代表活动质量。")
                    accentColor: AppTheme.warningColor

                    Repeater {
                        model: root.viewModel.opportunityAssessments

                        delegate: Rectangle {
                            id: opportunityCard
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            implicitHeight: opportunityContent.implicitHeight
                                + AppTheme.spacingMedium * 2
                            radius: AppTheme.radiusMedium
                            color: AppTheme.surfaceColor
                            border.color: modelData.totalScore >= 75
                                ? AppTheme.successColor
                                : (modelData.totalScore >= 55
                                    ? AppTheme.warningColor
                                    : AppTheme.borderColor)

                            HoverHandler {
                                cursorShape: Qt.PointingHandCursor
                            }
                            TapHandler {
                                onTapped: root.openSourceDetails(
                                    opportunityCard.modelData.sourceId)
                            }

                            ColumnLayout {
                                id: opportunityContent
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingMedium
                                spacing: AppTheme.spacingExtraSmall

                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        Layout.fillWidth: true
                                        text: opportunityCard.modelData.title
                                        color: AppTheme.textPrimaryColor
                                        font.bold: true
                                        wrapMode: Text.WordWrap
                                    }
                                    Label {
                                        text: opportunityCard.modelData.level
                                        color: opportunityCard.modelData.totalScore >= 75
                                            ? AppTheme.successColor
                                            : AppTheme.warningColor
                                        font.bold: true
                                    }
                                    Label {
                                        text: opportunityCard.modelData.totalScore
                                            + qsTr(" 分")
                                        color: AppTheme.textPrimaryColor
                                        font.pixelSize: AppTheme.fontSizeSubtitle
                                        font.bold: true
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("相关 %1/40　紧迫 %2/25　可行 %3/25　证据 %4/10")
                                        .arg(opportunityCard.modelData.relevanceScore)
                                        .arg(opportunityCard.modelData.urgencyScore)
                                        .arg(opportunityCard.modelData.feasibilityScore)
                                        .arg(opportunityCard.modelData.evidenceScore)
                                    color: AppTheme.textSecondaryColor
                                    wrapMode: Text.WordWrap
                                }
                                Label {
                                    visible: opportunityCard.modelData.reasons.length > 0
                                    Layout.fillWidth: true
                                    text: qsTr("加分依据：%1").arg(
                                        opportunityCard.modelData.reasons.join("；"))
                                    color: AppTheme.textSecondaryColor
                                    wrapMode: Text.WordWrap
                                }
                                Label {
                                    visible: opportunityCard.modelData.warnings.length > 0
                                    Layout.fillWidth: true
                                    text: qsTr("需核对：%1").arg(
                                        opportunityCard.modelData.warnings.join("；"))
                                    color: AppTheme.warningColor
                                    wrapMode: Text.WordWrap
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("资料 [S%1] · 点击查看原文")
                                        .arg(opportunityCard.modelData.sourceNumber)
                                    color: AppTheme.primaryColor
                                    font.pixelSize: AppTheme.fontSizeCaption
                                }
                            }
                        }
                    }
                }

                AppSectionCard {
                    Layout.fillWidth: true
                    Layout.columnSpan: root.narrowLayout ? 1 : 2
                    title: qsTr("AI 辅助分析")
                    description: root.viewModel.apiKeyConfigured
                        ? qsTr("已配置 %1。生成前会确认发送当前项目的资料，结果自动保存为新版本。")
                            .arg(root.viewModel.providerModelId)
                        : qsTr("支持 DeepSeek、OpenAI、Claude、Grok 和自定义兼容接口。API Key 仅保存在 Windows 凭据管理器。")
                    accentColor: AppTheme.primaryColor

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        AppButton {
                            text: qsTr("模型设置")
                            onClicked: {
                                providerBaseUrlField.text
                                    = root.viewModel.providerBaseUrl
                                providerModelField.text
                                    = root.viewModel.providerModelId
                                providerProtocolCombo.currentIndex
                                    = root.viewModel.providerProtocolIndex
                                providerWorkspaceField.text
                                    = root.viewModel.providerWorkspaceId
                                providerApiKeyField.clear()
                                providerDialog.open()
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            text: root.viewModel.apiKeyConfigured
                                ? qsTr("API Key 已安全保存")
                                : qsTr("尚未保存 API Key")
                            color: root.viewModel.apiKeyConfigured
                                ? AppTheme.successColor
                                : AppTheme.textSecondaryColor
                            wrapMode: Text.WordWrap
                        }
                    }

                    AppTextArea {
                        id: analysisInstructionsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 96
                        enabled: !root.viewModel.aiBusy
                        placeholderText: qsTr("可选：补充你的偏好，例如优先准备软件开发实习、每周最多投入 10 小时")
                        wrapMode: TextEdit.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        Label {
                            Layout.fillWidth: true
                            text: root.viewModel.aiBusy
                                ? qsTr("模型正在分析，请保持网络连接……")
                                : qsTr("发送上限约 12 万字符；超出部分不会上传。")
                            color: AppTheme.textSecondaryColor
                            wrapMode: Text.WordWrap
                        }
                        AppButton {
                            visible: root.viewModel.aiBusy
                            text: qsTr("取消")
                            onClicked: root.viewModel.cancelGeneration()
                        }
                        AppButton {
                            text: qsTr("生成规划")
                            highlighted: true
                            enabled: !root.viewModel.aiBusy
                                && root.viewModel.hasSources
                            onClicked: {
                                if (!root.viewModel.apiKeyConfigured) {
                                    providerBaseUrlField.text
                                        = root.viewModel.providerBaseUrl
                                    providerModelField.text
                                        = root.viewModel.providerModelId
                                    providerProtocolCombo.currentIndex
                                        = root.viewModel.providerProtocolIndex
                                    providerWorkspaceField.text
                                        = root.viewModel.providerWorkspaceId
                                    providerApiKeyField.clear()
                                    providerDialog.open()
                                    return
                                }
                                root.pendingAnalysisInstructions
                                    = analysisInstructionsArea.text
                                analysisConsentDialog.open()
                            }
                        }
                    }
                }

                AppSectionCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: root.narrowLayout ? -1 : 300
                    title: qsTr("规划项目")
                    description: root.viewModel.persistentStorage
                        ? qsTr("项目和资料保存在 StuKit 本地数据库中。")
                        : qsTr("数据库不可用，本次内容只保留在当前运行会话。")

                    AppButton {
                        Layout.alignment: Qt.AlignLeft
                        text: qsTr("编辑当前规划")
                        onClicked: {
                            root.viewModel.clearMessages()
                            root.openProjectEditor()
                        }
                    }

                    Repeater {
                        model: root.viewModel.projects

                        delegate: Rectangle {
                            id: projectCard
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            implicitHeight: projectContent.implicitHeight
                                + AppTheme.spacingMedium * 2
                            radius: AppTheme.radiusMedium
                            color: modelData.selected
                                ? AppTheme.selectionColor
                                : (projectHover.hovered
                                    ? AppTheme.surfaceHoverColor
                                    : AppTheme.surfaceColor)
                            border.width: modelData.selected ? 2 : 1
                            border.color: modelData.selected
                                ? AppTheme.primaryColor : AppTheme.borderColor

                            HoverHandler {
                                id: projectHover
                                cursorShape: Qt.PointingHandCursor
                            }
                            TapHandler {
                                onTapped: root.viewModel.selectProject(
                                    projectCard.modelData.id)
                            }
                            ColumnLayout {
                                id: projectContent
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingMedium
                                spacing: AppTheme.spacingExtraSmall

                                Label {
                                    Layout.fillWidth: true
                                    text: projectCard.modelData.title
                                    color: AppTheme.textPrimaryColor
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: projectCard.modelData.objective
                                    color: AppTheme.textSecondaryColor
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                AppSectionCard {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    Layout.preferredWidth: root.narrowLayout ? -1 : 700
                    title: root.viewModel.selectedProjectTitle.length > 0
                        ? qsTr("资料库 · %1").arg(
                            root.viewModel.selectedProjectTitle)
                        : qsTr("资料库")
                    description: qsTr("资料目前只用于本地整理；接入模型前会再次让你确认发送范围。")
                    accentColor: AppTheme.primaryColor

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        AppButton {
                            Layout.fillWidth: root.narrowLayout
                            text: qsTr("粘贴资料")
                            highlighted: true
                            onClicked: {
                                root.viewModel.clearMessages()
                                root.resetPasteForm()
                                pasteDialog.open()
                            }
                        }
                        AppButton {
                            Layout.fillWidth: root.narrowLayout
                            text: qsTr("导入文件")
                            onClicked: {
                                root.viewModel.clearMessages()
                                importTitleField.clear()
                                importKindCombo.currentIndex = 0
                                importOptionsDialog.open()
                            }
                        }
                    }

                    Label {
                        visible: !root.viewModel.hasSources
                        Layout.fillWidth: true
                        Layout.topMargin: AppTheme.spacingMedium
                        text: qsTr("当前规划还没有资料。建议首先加入培养方案，再加入竞赛、训练营或个人目标说明。")
                        color: AppTheme.textSecondaryColor
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: root.viewModel.sources

                        delegate: Rectangle {
                            id: sourceCard
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            implicitHeight: sourceContent.implicitHeight
                                + AppTheme.spacingMedium * 2
                            radius: AppTheme.radiusMedium
                            color: AppTheme.surfaceColor
                            border.color: AppTheme.borderColor

                            HoverHandler {
                                id: sourceHover
                                cursorShape: Qt.PointingHandCursor
                            }
                            TapHandler {
                                onTapped: root.openSourceDetails(
                                    sourceCard.modelData.id)
                            }

                            ColumnLayout {
                                id: sourceContent
                                anchors.fill: parent
                                anchors.margins: AppTheme.spacingMedium
                                spacing: AppTheme.spacingExtraSmall

                                RowLayout {
                                    Layout.fillWidth: true
                                    Label {
                                        Layout.fillWidth: true
                                        text: sourceCard.modelData.title
                                        color: AppTheme.textPrimaryColor
                                        font.bold: true
                                        wrapMode: Text.WordWrap
                                    }
                                    Label {
                                        text: sourceCard.modelData.kind
                                        color: AppTheme.primaryColor
                                        font.pixelSize: AppTheme.fontSizeCaption
                                    }
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: sourceCard.modelData.preview
                                    color: AppTheme.textSecondaryColor
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 3
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("%1 · %2 · 哈希 %3").arg(
                                        sourceCard.modelData.format).arg(
                                        sourceCard.modelData.importedAt).arg(
                                        sourceCard.modelData.hash)
                                    color: AppTheme.textSecondaryColor
                                    font.pixelSize: AppTheme.fontSizeCaption
                                    elide: Text.ElideRight
                                }
                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("点击查看完整原文")
                                    color: AppTheme.primaryColor
                                    font.pixelSize: AppTheme.fontSizeCaption
                                }
                            }
                        }
                    }
                }

                AppSectionCard {
                    Layout.fillWidth: true
                    Layout.columnSpan: root.narrowLayout ? 1 : 2
                    title: qsTr("规划文档")
                    description: root.viewModel.hasPlan
                        ? qsTr("当前查看版本 %1；每次保存都会创建新版本，不覆盖历史。")
                            .arg(root.viewModel.planRevisionNumber)
                        : qsTr("使用 Markdown 维护长期目标、里程碑、行动和风险。")
                    accentColor: AppTheme.successColor

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        AppButton {
                            visible: !root.viewModel.hasPlan
                            text: qsTr("使用基础模板")
                            onClicked: planEditor.text =
                                "# " + root.viewModel.selectedProjectTitle
                                + "\n\n## 长期目标\n\n"
                                + "## 培养方案差距\n\n"
                                + "## 推荐活动\n\n"
                                + "## 本学期路线图\n\n"
                                + "## 本周行动\n\n- [ ] \n\n"
                                + "## 风险与假设\n\n"
                                + "## 资料来源\n"
                        }
                        Item { Layout.fillWidth: true }
                        AppButton {
                            text: qsTr("创建待办")
                            enabled: planEditor.text.trim().length > 0
                            onClicked: {
                                if (root.viewModel.preparePlanActions(
                                        planEditor.text)) {
                                    actionPublishDialog.open()
                                }
                            }
                        }
                        AppButton {
                            text: qsTr("保存新版本")
                            highlighted: true
                            onClicked: root.viewModel.savePlan(planEditor.text)
                        }
                        AppButton {
                            text: qsTr("导出 Markdown")
                            enabled: planEditor.text.trim().length > 0
                            onClicked: planExportDialog.open()
                        }
                    }

                    SplitView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 440
                        orientation: root.narrowLayout
                            ? Qt.Vertical : Qt.Horizontal

                        Item {
                            SplitView.fillWidth: true
                            SplitView.fillHeight: true
                            SplitView.minimumWidth: 260
                            SplitView.minimumHeight: 180

                            AppTextArea {
                                id: planEditor
                                anchors.fill: parent
                                text: root.viewModel.planMarkdown
                                placeholderText: qsTr("在这里编写 Markdown 规划")
                                wrapMode: TextEdit.Wrap
                            }
                        }

                        ScrollView {
                            SplitView.fillWidth: true
                            SplitView.fillHeight: true
                            SplitView.minimumWidth: 260
                            SplitView.minimumHeight: 180
                            clip: true
                            contentWidth: availableWidth

                            Text {
                                width: parent.width
                                padding: AppTheme.spacingMedium
                                text: root.safeMarkdownPreview(planEditor.text)
                                textFormat: Text.MarkdownText
                                color: AppTheme.textPrimaryColor
                                wrapMode: Text.WordWrap
                                onLinkActivated: link =>
                                    Qt.openUrlExternally(link)
                            }
                        }
                    }

                    Label {
                        visible: root.viewModel.planRevisions.length > 0
                        Layout.fillWidth: true
                        Layout.topMargin: AppTheme.spacingMedium
                        text: qsTr("历史版本")
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: AppTheme.spacingSmall

                        Repeater {
                            model: root.viewModel.planRevisions

                            delegate: AppButton {
                                required property var modelData
                                required property int index
                                text: qsTr("v%1 · %2").arg(
                                    modelData.revisionNumber || index + 1).arg(
                                    modelData.createdAt)
                                highlighted: modelData.selected
                                onClicked: {
                                    if (root.viewModel.selectPlanRevision(
                                            modelData.id)) {
                                        planEditor.text
                                            = root.viewModel.planMarkdown
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: AppTheme.spacingLarge }
        }
    }

    AppDialog {
        id: projectDialog
        parent: root
        title: qsTr("新建规划")
        standardButtons: Dialog.Cancel | Dialog.Save
        viewportWidth: root.width
        viewportHeight: root.height
        enterActionEnabled: true
        onEnterActionRequested: accept()
        onAccepted: {
            if (!root.viewModel.createProject(
                    projectTitleField.text,
                    projectObjectiveField.text,
                    projectConstraintsField.text)) {
                open()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("规划名称")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: projectTitleField
            Layout.fillWidth: true
            placeholderText: qsTr("例如：本科阶段成长规划")
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            text: qsTr("主要目标")
            color: AppTheme.textPrimaryColor
        }
        AppTextArea {
            id: projectObjectiveField
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            placeholderText: qsTr("说明你希望达到的学习或生活目标")
            wrapMode: TextEdit.Wrap
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            text: qsTr("限制条件（可选）")
            color: AppTheme.textPrimaryColor
        }
        AppTextArea {
            id: projectConstraintsField
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            placeholderText: qsTr("例如：每周投入不超过 12 小时、预算不超过 1000 元")
            wrapMode: TextEdit.Wrap
        }
    }

    AppDialog {
        id: editProjectDialog
        parent: root
        title: qsTr("编辑当前规划")
        standardButtons: Dialog.Cancel | Dialog.Save
        viewportWidth: root.width
        viewportHeight: root.height
        enterActionEnabled: true
        onEnterActionRequested: accept()
        onAccepted: {
            if (!root.viewModel.updateSelectedProject(
                    editProjectTitleField.text,
                    editProjectObjectiveField.text,
                    editProjectConstraintsField.text)) {
                open()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("规划名称")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: editProjectTitleField
            Layout.fillWidth: true
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            text: qsTr("主要目标")
            color: AppTheme.textPrimaryColor
        }
        AppTextArea {
            id: editProjectObjectiveField
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            placeholderText: qsTr("评分会使用这里的目标判断活动相关度")
            wrapMode: TextEdit.Wrap
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            text: qsTr("限制条件（可选）")
            color: AppTheme.textPrimaryColor
        }
        AppTextArea {
            id: editProjectConstraintsField
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            placeholderText: qsTr("例如：每周不超过 10 小时、预算不超过 800 元")
            wrapMode: TextEdit.Wrap
        }
    }

    AppDialog {
        id: actionPublishDialog
        parent: root
        title: qsTr("确认创建待办")
        standardButtons: Dialog.Cancel | Dialog.Ok
        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 620
        enterActionEnabled: true
        onEnterActionRequested: accept()
        onAccepted: {
            if (!root.viewModel.publishPreparedActions())
                open()
        }
        onRejected: root.viewModel.clearPreparedActions()

        Label {
            Layout.fillWidth: true
            text: qsTr("将以下 %1 条 - [ ] 行动项创建为“规划”分类的待办。创建后可到待办模块设置截止时间、提醒和优先级。")
                .arg(root.viewModel.actionDraftCount)
            color: AppTheme.textSecondaryColor
            wrapMode: Text.WordWrap
        }

        Repeater {
            model: root.viewModel.actionDrafts

            delegate: Rectangle {
                required property var modelData
                required property int index
                Layout.fillWidth: true
                implicitHeight: actionTitle.implicitHeight
                    + AppTheme.spacingMedium * 2
                radius: AppTheme.radiusMedium
                color: AppTheme.surfaceColor
                border.color: AppTheme.borderColor

                Label {
                    id: actionTitle
                    anchors.fill: parent
                    anchors.margins: AppTheme.spacingMedium
                    text: qsTr("%1. %2").arg(index + 1).arg(modelData.title)
                    color: AppTheme.textPrimaryColor
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    AppDialog {
        id: pasteDialog
        parent: root
        title: qsTr("粘贴资料")
        standardButtons: Dialog.Cancel | Dialog.Save
        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 680
        onAccepted: {
            if (!root.viewModel.addPastedSource(
                    sourceTitleField.text,
                    sourceKindCombo.currentIndex,
                    sourceFormatCombo.currentIndex,
                    sourceContentArea.text)) {
                open()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("资料标题")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: sourceTitleField
            Layout.fillWidth: true
            placeholderText: qsTr("例如：2026 版培养方案")
        }
        GridLayout {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            columns: root.narrowLayout ? 1 : 2
            columnSpacing: AppTheme.spacingSmall
            rowSpacing: AppTheme.spacingSmall

            AppComboBox {
                id: sourceKindCombo
                Layout.fillWidth: true
                model: root.viewModel.sourceKindOptions
            }
            AppComboBox {
                id: sourceFormatCombo
                Layout.fillWidth: true
                model: root.viewModel.sourceFormatOptions
            }
        }
        AppTextArea {
            id: sourceContentArea
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            Layout.preferredHeight: 260
            placeholderText: qsTr("在这里粘贴资料正文")
            wrapMode: TextEdit.Wrap
        }
    }

    AppDialog {
        id: importOptionsDialog
        parent: root
        title: qsTr("导入文件")
        standardButtons: Dialog.Cancel | Dialog.Ok
        viewportWidth: root.width
        viewportHeight: root.height
        onAccepted: {
            root.pendingImportTitle = importTitleField.text
            root.pendingImportKind = importKindCombo.currentIndex
            sourceFileDialog.open()
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("资料标题（留空则使用文件名）")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: importTitleField
            Layout.fillWidth: true
            placeholderText: qsTr("可选")
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            text: qsTr("资料类型")
            color: AppTheme.textPrimaryColor
        }
        AppComboBox {
            id: importKindCombo
            Layout.fillWidth: true
            model: root.viewModel.sourceKindOptions
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            text: qsTr("支持 UTF-8 编码的 .txt、.md、.markdown 和 .csv，单个文件不超过 2 MiB。")
            color: AppTheme.textSecondaryColor
            wrapMode: Text.WordWrap
        }
    }

    AppDialog {
        id: sourceDetailsDialog
        parent: root
        title: qsTr("资料原文")
        standardButtons: Dialog.Cancel | Dialog.Save
        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 820
        onRejected: root.viewModel.clearSelectedSource()
        onAccepted: {
            if (!root.viewModel.updateSelectedSource(
                    sourceDetailTitleField.text,
                    sourceDetailKindCombo.currentIndex,
                    sourceDetailFormatCombo.currentIndex,
                    sourceDetailContentArea.text)) {
                open()
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.viewModel.selectedSourceFileName.length > 0
            text: qsTr("原文件：%1").arg(
                root.viewModel.selectedSourceFileName)
            color: AppTheme.textSecondaryColor
            elide: Text.ElideMiddle
        }
        AppTextField {
            id: sourceDetailTitleField
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            placeholderText: qsTr("资料标题")
        }
        GridLayout {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            columns: root.narrowLayout ? 1 : 2
            columnSpacing: AppTheme.spacingSmall
            rowSpacing: AppTheme.spacingSmall

            AppComboBox {
                id: sourceDetailKindCombo
                Layout.fillWidth: true
                model: root.viewModel.sourceKindOptions
            }
            AppComboBox {
                id: sourceDetailFormatCombo
                Layout.fillWidth: true
                model: root.viewModel.sourceFormatOptions
            }
        }
        TabBar {
            id: sourceDetailTabs
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium

            TabButton { text: qsTr("原文 / 编辑") }
            TabButton {
                text: qsTr("Markdown 预览")
                enabled: sourceDetailFormatCombo.currentIndex === 1
            }
        }
        StackLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(420,
                Math.max(220, root.height * 0.45))
            currentIndex: sourceDetailTabs.currentIndex

            AppTextArea {
                id: sourceDetailContentArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: TextEdit.Wrap
                placeholderText: qsTr("资料正文")
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth

                Text {
                    width: parent.width
                    padding: AppTheme.spacingMedium
                    text: root.safeMarkdownPreview(
                        sourceDetailContentArea.text)
                    textFormat: Text.MarkdownText
                    color: AppTheme.textPrimaryColor
                    wrapMode: Text.WordWrap
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium

            AppButton {
                text: qsTr("删除资料")
                onClicked: deleteSourceDialog.open()
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("保存后内容哈希会重新计算")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeCaption
            }
        }
    }

    AppDialog {
        id: deleteSourceDialog
        parent: root
        title: qsTr("删除资料")
        standardButtons: Dialog.No | Dialog.Yes
        viewportWidth: root.width
        viewportHeight: root.height
        onAccepted: {
            if (root.viewModel.deleteSelectedSource())
                sourceDetailsDialog.close()
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("确定删除“%1”吗？该操作会从本地资料库移除正文。")
                .arg(root.viewModel.selectedSourceTitle)
            color: AppTheme.textPrimaryColor
            wrapMode: Text.WordWrap
        }
    }

    AppDialog {
        id: providerDialog
        parent: root
        title: qsTr("模型服务设置")
        standardButtons: Dialog.Cancel | Dialog.Save
        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 620
        onAccepted: {
            if (!root.viewModel.saveProviderSettings(
                    providerBaseUrlField.text,
                    providerModelField.text,
                    providerApiKeyField.text,
                    providerProtocolCombo.currentIndex,
                    providerWorkspaceField.text)) {
                open()
            } else {
                providerApiKeyField.clear()
            }
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("支持 DeepSeek、OpenAI、Grok 的 OpenAI 兼容协议，以及 Claude 的 Anthropic Messages 协议。API Key 按协议分开保存在 Windows 凭据管理器，不进入数据库和 settings.ini。")
            color: AppTheme.textSecondaryColor
            wrapMode: Text.WordWrap
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            text: qsTr("请求协议")
            color: AppTheme.textPrimaryColor
        }
        AppComboBox {
            id: providerProtocolCombo
            Layout.fillWidth: true
            model: root.viewModel.providerProtocolOptions
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            text: qsTr("API 基础地址")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: providerBaseUrlField
            Layout.fillWidth: true
            placeholderText: qsTr("https://api.deepseek.com")
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            text: qsTr("模型名称")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: providerModelField
            Layout.fillWidth: true
            placeholderText: qsTr("deepseek-flash")
        }
        Label {
            visible: providerProtocolCombo.currentIndex === 1
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            text: qsTr("Anthropic Workspace ID（可选）")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: providerWorkspaceField
            visible: providerProtocolCombo.currentIndex === 1
            Layout.fillWidth: true
            placeholderText: qsTr("只有未绑定工作区的 Key 需要填写")
        }
        Label {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingSmall
            text: root.viewModel.apiKeyConfigured
                ? qsTr("API Key（留空表示继续使用已保存的密钥）")
                : qsTr("API Key")
            color: AppTheme.textPrimaryColor
        }
        AppTextField {
            id: providerApiKeyField
            Layout.fillWidth: true
            echoMode: TextInput.Password
            passwordCharacter: "●"
            placeholderText: root.viewModel.apiKeyConfigured
                ? qsTr("已安全保存") : qsTr("输入服务商提供的 API Key")
        }
        Flow {
            Layout.fillWidth: true
            Layout.topMargin: AppTheme.spacingMedium
            spacing: AppTheme.spacingSmall

            AppButton {
                text: qsTr("DeepSeek")
                onClicked: {
                    providerProtocolCombo.currentIndex = 0
                    providerWorkspaceField.clear()
                    providerBaseUrlField.text = "https://api.deepseek.com"
                    providerModelField.text = "deepseek-flash"
                }
            }
            AppButton {
                text: qsTr("OpenAI")
                onClicked: {
                    providerProtocolCombo.currentIndex = 0
                    providerWorkspaceField.clear()
                    providerBaseUrlField.text = "https://api.openai.com/v1"
                    providerModelField.text = "gpt-5"
                }
            }
            AppButton {
                text: qsTr("Claude")
                onClicked: {
                    providerProtocolCombo.currentIndex = 1
                    providerBaseUrlField.text = "https://api.anthropic.com"
                    providerModelField.text = "claude-sonnet-5-5"
                }
            }
            AppButton {
                text: qsTr("Grok")
                onClicked: {
                    providerProtocolCombo.currentIndex = 0
                    providerWorkspaceField.clear()
                    providerBaseUrlField.text = "https://api.x.ai/v1"
                    providerModelField.text = "grok-4.7"
                }
            }
            AppButton {
                visible: root.viewModel.apiKeyConfigured
                    && providerProtocolCombo.currentIndex
                        === root.viewModel.providerProtocolIndex
                text: qsTr("删除已保存密钥")
                onClicked: {
                    if (root.viewModel.removeApiKey())
                        providerApiKeyField.clear()
                }
            }
        }
    }

    AppDialog {
        id: analysisConsentDialog
        parent: root
        title: qsTr("确认发送资料")
        standardButtons: Dialog.No | Dialog.Yes
        viewportWidth: root.width
        viewportHeight: root.height
        preferredWidth: 620
        onAccepted: root.viewModel.generatePlan(
            root.pendingAnalysisInstructions)

        Label {
            Layout.fillWidth: true
            text: qsTr("将把“%1”的目标、限制条件和当前 %2 份资料正文发送到：\n%3\n模型：%4\n\n资料可能包含个人信息，请确认内容适合交给该服务商处理。原始文件本身不会上传，发送的是导入后的文本副本。")
                .arg(root.viewModel.selectedProjectTitle)
                .arg(root.viewModel.sources.length)
                .arg(root.viewModel.providerBaseUrl)
                .arg(root.viewModel.providerModelId)
            color: AppTheme.textPrimaryColor
            wrapMode: Text.WordWrap
        }
    }

    FileDialog {
        id: planExportDialog
        title: qsTr("导出规划 Markdown")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "md"
        nameFilters: [qsTr("Markdown 文件 (*.md)")]
        onAccepted: root.viewModel.exportPlan(
            selectedFile, planEditor.text)
    }

    FileDialog {
        id: sourceFileDialog
        title: qsTr("选择规划资料")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("支持的资料 (*.txt *.md *.markdown *.csv)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: root.viewModel.importSource(
            selectedFile,
            root.pendingImportTitle,
            root.pendingImportKind)
    }
}
