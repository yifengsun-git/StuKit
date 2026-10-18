import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property var viewModel

    readonly property int pagePadding: AppTheme.pagePadding(width)
    readonly property bool hasViewModel: viewModel !== null

    background: Rectangle {
        color: AppTheme.backgroundColor
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
                760))
            spacing: AppTheme.spacingLarge

            Item {
                Layout.preferredHeight: AppTheme.spacingSmall
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Windows 更新")
                color: AppTheme.textPrimaryColor
                font.pixelSize: AppTheme.fontSizeHeading
                font.bold: true
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("从 StuKit 的 GitHub Release 检查并下载安装新版本。")
                color: AppTheme.textSecondaryColor
                font.pixelSize: AppTheme.fontSizeBody
                wrapMode: Text.WordWrap
            }

            AppSectionCard {
                Layout.fillWidth: true
                title: qsTr("版本信息")

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingMedium

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("当前版本")
                        color: AppTheme.textSecondaryColor
                    }

                    Label {
                        text: root.hasViewModel
                            ? "v" + root.viewModel.currentVersion : "-"
                        color: AppTheme.textPrimaryColor
                        font.bold: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: root.hasViewModel
                        && root.viewModel.latestVersion.length > 0
                    spacing: AppTheme.spacingMedium

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("最新版本")
                        color: AppTheme.textSecondaryColor
                    }

                    Label {
                        text: visible
                            ? "v" + root.viewModel.latestVersion : ""
                        color: AppTheme.primaryColor
                        font.bold: true
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.hasViewModel
                        && root.viewModel.publishedAtText.length > 0
                    text: visible
                        ? qsTr("发布时间：%1").arg(
                            root.viewModel.publishedAtText) : ""
                    color: AppTheme.textSecondaryColor
                    font.pixelSize: AppTheme.fontSizeCaption
                }
            }

            AppSectionCard {
                Layout.fillWidth: true
                title: qsTr("更新状态")
                accentColor: root.hasViewModel
                    && root.viewModel.errorMessage.length > 0
                    ? AppTheme.errorColor : AppTheme.borderColor

                BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    visible: running
                    running: root.hasViewModel && root.viewModel.busy
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.hasViewModel
                        && root.viewModel.statusMessage.length > 0
                    text: visible ? root.viewModel.statusMessage : ""
                    color: AppTheme.textPrimaryColor
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.hasViewModel
                        && root.viewModel.errorMessage.length > 0
                    text: visible ? root.viewModel.errorMessage : ""
                    color: AppTheme.errorColor
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                ProgressBar {
                    Layout.fillWidth: true
                    visible: root.hasViewModel
                        && (root.viewModel.downloadProgress > 0
                            || root.viewModel.readyToInstall)
                    from: 0
                    to: 1
                    value: root.hasViewModel
                        ? root.viewModel.downloadProgress : 0
                }

                Label {
                    Layout.fillWidth: true
                    visible: root.hasViewModel
                        && root.viewModel.packageSizeText.length > 0
                    text: visible
                        ? qsTr("安装包大小：%1").arg(
                            root.viewModel.packageSizeText) : ""
                    color: AppTheme.textSecondaryColor
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: AppTheme.fontSizeCaption
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: AppTheme.spacingSmall

                    AppButton {
                        Layout.fillWidth: true
                        text: qsTr("检查更新")
                        enabled: root.hasViewModel && !root.viewModel.busy
                        onClicked: root.viewModel.checkForUpdates()
                    }

                    AppButton {
                        Layout.fillWidth: true
                        visible: root.hasViewModel
                            && root.viewModel.canDownload
                        highlighted: true
                        text: qsTr("下载更新")
                        onClicked: root.viewModel.downloadUpdate()
                    }

                    AppButton {
                        Layout.fillWidth: true
                        visible: root.hasViewModel
                            && root.viewModel.readyToInstall
                        highlighted: true
                        text: qsTr("安装并退出")
                        onClicked: root.viewModel.installUpdate()
                    }

                    AppButton {
                        visible: root.hasViewModel && root.viewModel.busy
                        text: qsTr("取消")
                        flat: true
                        onClicked: root.viewModel.cancel()
                    }
                }
            }

            AppSectionCard {
                Layout.fillWidth: true
                visible: root.hasViewModel && root.viewModel.updateAvailable
                title: qsTr("版本说明")
                description: root.viewModel.releaseNotes.length > 0
                    ? root.viewModel.releaseNotes
                    : qsTr("该版本没有填写更新说明。")

                AppButton {
                    text: qsTr("在 GitHub 查看 Release")
                    flat: true
                    onClicked: root.viewModel.openReleasePage()
                }
            }

            AppSectionCard {
                Layout.fillWidth: true
                title: qsTr("数据安全")
                description: qsTr(
                    "更新只替换程序文件。账本、课表和待办数据保存在 Windows AppData 中，不会随覆盖安装被删除。重要更新前仍建议手动备份数据库。")
                accentColor: AppTheme.successColor
            }

            Item {
                Layout.preferredHeight: AppTheme.spacingLarge
            }
        }
    }
}
