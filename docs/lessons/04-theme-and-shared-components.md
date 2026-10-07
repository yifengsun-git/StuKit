# 第四课：建立主题与通用组件

## 本课目标

本课建立最小主题系统，并将记账本和课表的重复占位页面提取为通用组件。

完成后，你应该理解：

- 主题状态和设计令牌分别是什么。
- QML Singleton 如何提供全局 UI 配置。
- 为什么页面不应到处直接填写颜色和间距数值。
- 如何通过组件属性复用界面结构。
- `required property` 如何强制调用者提供必要信息。

本课不会保存用户的主题选择。应用重启后恢复浅色主题是预期行为，主题持久化
会在设置模块中实现。

## 1. 三个核心概念

### 主题状态

主题状态表示当前使用浅色还是深色。本课使用：

```qml
property bool darkMode: false
```

### 设计令牌

设计令牌是具有统一名称的视觉值，例如：

```text
backgroundColor  页面背景色
textPrimaryColor 主要文字颜色
spacingMedium    标准间距
fontSizeHeading  页面标题字号
controlHeight    常用控件高度
```

页面使用令牌而不是直接写大量 `#FFFFFF`、`16`、`24`。调整整体视觉风格时，
只需修改令牌定义。

### 通用组件

通用组件封装重复的界面结构。本课的记账与课表占位页结构相同，只有标题和
说明不同，因此提取为 `FeaturePlaceholder`。

## 2. 本课完成后的新增文件

```text
src/shared/qml/
  theme/
    AppTheme.qml
  components/
    FeaturePlaceholder.qml
```

`shared` 中只放已经被多个功能模块共同使用的稳定内容。本课的主题和占位页
都满足这个条件。

## 3. 创建 AppTheme 单例

创建 `src/shared/qml/theme/AppTheme.qml`：

```qml
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
```

`pragma Singleton` 表示整个 QML 引擎中只有一个 AppTheme 对象。所有页面读取
同一个 `darkMode`，切换一次即可更新所有依赖它的颜色绑定。

颜色和尺寸名称描述用途，不描述具体数值。例如使用 `spacingMedium`，而不是
`spacing16`，以后才能在不改名称的情况下调整设计。

## 4. 在 CMake 中注册主题和组件

在 `STUKIT_QML_FILES` 中增加：

```cmake
    src/shared/qml/theme/AppTheme.qml
    src/shared/qml/components/FeaturePlaceholder.qml
```

在现有资源别名配置后、`qt_add_qml_module` 前增加：

```cmake
set_source_files_properties(
    src/shared/qml/theme/AppTheme.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_RESOURCE_ALIAS AppTheme.qml
)

set_source_files_properties(
    src/shared/qml/components/FeaturePlaceholder.qml
    PROPERTIES QT_RESOURCE_ALIAS FeaturePlaceholder.qml
)
```

`pragma Singleton` 是 QML 文件中的声明，`QT_QML_SINGLETON_TYPE TRUE` 是构建
系统中的注册，两者都需要存在。

如果 Qt Creator 创建文件时自动向 `qt_add_qml_module` 添加了额外的
`QML_FILES` 行，请删除自动添加的重复项。最终仍然只通过
`${STUKIT_QML_FILES}` 注册一次。

## 5. 创建通用占位页

创建 `src/shared/qml/components/FeaturePlaceholder.qml`：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property string heading
    required property string description

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: AppTheme.spacingSmall

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.heading
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeDisplay
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.description
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody
        }
    }
}
```

`required property` 表示创建组件时必须提供该属性。如果忘记填写 heading 或
description，QML 会直接给出错误，而不是静默显示一个不完整页面。

## 6. 简化两个功能占位页

将 `AccountingHomePage.qml` 改为：

```qml
import QtQuick

FeaturePlaceholder {
    heading: qsTr("记账本")
    description: qsTr("记账功能将在后续课程中实现")
}
```

将 `ScheduleHomePage.qml` 改为：

```qml
import QtQuick

FeaturePlaceholder {
    heading: qsTr("课表")
    description: qsTr("课表功能将在后续课程中实现")
}
```

这两个文件仍然保留，因为它们代表不同功能模块的页面入口。未来加入真实功能
时可以分别扩展，而当前重复布局由 FeaturePlaceholder 统一管理。

## 7. 给首页应用设计令牌

将 `HomePage.qml` 改为：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    signal accountingRequested()
    signal scheduleRequested()

    background: Rectangle {
        color: AppTheme.backgroundColor
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: AppTheme.spacingLarge

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("欢迎使用 StuKit")
            color: AppTheme.textPrimaryColor
            font.pixelSize: AppTheme.fontSizeDisplay
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("选择一个功能开始使用")
            color: AppTheme.textSecondaryColor
            font.pixelSize: AppTheme.fontSizeBody
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: AppTheme.spacingMedium

            Button {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("记账本")
                onClicked: root.accountingRequested()
            }

            Button {
                Layout.preferredWidth: 160
                Layout.preferredHeight: AppTheme.controlHeight
                text: qsTr("课表")
                onClicked: root.scheduleRequested()
            }
        }
    }
}
```

按钮宽度 `160` 是这个页面的具体布局选择，因此可以保留在页面中；颜色、常用
间距、字体和控件高度属于全局视觉规则，所以使用 AppTheme。

## 8. 给应用外壳应用主题

在 `Main.qml` 的 `title` 后增加窗口颜色和调色板：

```qml
    color: AppTheme.backgroundColor

    palette.window: AppTheme.backgroundColor
    palette.windowText: AppTheme.textPrimaryColor
    palette.base: AppTheme.surfaceColor
    palette.text: AppTheme.textPrimaryColor
    palette.button: AppTheme.surfaceVariantColor
    palette.buttonText: AppTheme.textPrimaryColor
    palette.highlight: AppTheme.primaryColor
    palette.highlightedText: AppTheme.textOnPrimaryColor
```

在 `ToolBar` 内、`RowLayout` 之前增加：

```qml
        background: Rectangle {
            color: AppTheme.surfaceColor
            border.color: AppTheme.borderColor
            border.width: 1
        }
```

将 ToolBar 中的布局边距：

```qml
anchors.margins: 4
```

替换为：

```qml
anchors.margins: AppTheme.spacingExtraSmall
```

给 ToolBar 的标题 Label 增加：

```qml
color: AppTheme.textPrimaryColor
```

在 `Drawer` 的 `height` 后增加：

```qml
        background: Rectangle {
            color: AppTheme.surfaceColor
        }
```

在 Drawer 内的 `Pane` 中增加透明背景：

```qml
            background: Rectangle {
                color: "transparent"
            }
```

将 Drawer 中 ColumnLayout 的：

```qml
spacing: 8
```

替换为：

```qml
spacing: AppTheme.spacingSmall
```

给 Drawer 中的 StuKit 标题 Label 增加：

```qml
color: AppTheme.textPrimaryColor
```

在 Drawer 的三个导航 Button 后、占位 Item 前增加主题开关：

```qml
                Switch {
                    Layout.fillWidth: true
                    text: qsTr("深色模式")
                    checked: AppTheme.darkMode
                    onToggled: AppTheme.darkMode = checked
                }
```

完整顺序应为：

```text
StuKit 标题
首页按钮
记账本按钮
课表按钮
深色模式开关
填充剩余高度的 Item
```

## 9. 为什么主题属于表现层

颜色、字号、间距和深色模式只影响界面展示，不改变账单金额、课程冲突等业务
规则。因此 AppTheme 属于 `shared/qml`，不能放进 Domain 或 Application。

主题开关目前也是纯 UI 状态。将来保存设置时，QML 会通过设置服务读写用户
偏好，但主题颜色定义仍留在表现层。

## 10. 构建与人工验收

新增 QML 文件后，先重新运行 CMake，再进行 Debug 构建。

依次检查：

- 默认使用浅色主题。
- 打开 Drawer 并启用深色模式。
- 首页、记账本、课表背景和文字颜色同时改变。
- 在不同页面间切换时，深色模式保持不变。
- 关闭程序重新启动后恢复浅色主题，这是本课预期行为。
- 两个功能占位页内容与重构前一致。
- 应用输出没有 QML 错误或未定义属性警告。

## 11. 常见错误

### AppTheme is not a type 或 AppTheme is not defined

检查以下三项：

- AppTheme.qml 是否在 `STUKIT_QML_FILES` 中。
- 是否设置 `QT_QML_SINGLETON_TYPE TRUE`。
- 新增文件后是否重新运行 CMake。

### qmldir 中存在重复类型

检查 Qt Creator 是否在 `${STUKIT_QML_FILES}` 之外，又自动添加了单独的
`QML_FILES` 行。

### Required property heading was not initialized

创建 FeaturePlaceholder 时遗漏了 heading 或 description。

### 深色模式只有部分区域变化

检查对应控件是否仍然写死颜色，或者是否没有继承 ApplicationWindow 的
palette。先查看应用输出，不要通过到处复制深色颜色值解决。

## 12. 本课验收清单

- [ ] AppTheme 以 QML Singleton 注册。
- [ ] 浅色和深色令牌能够自动切换。
- [ ] 首页使用主题颜色、间距和字号。
- [ ] 两个占位页复用 FeaturePlaceholder。
- [ ] Drawer 可以切换深色模式。
- [ ] 页面切换不会丢失当前主题状态。
- [ ] 重启后恢复浅色主题。
- [ ] CMake 中没有重复注册 QML 文件。
- [ ] 应用输出没有 QML 错误。
- [ ] Git 状态中没有构建产物。

## 13. 完成后反馈

请提供：

```text
CMake 配置：成功 / 失败
Debug 构建：成功 / 失败
浅色主题：正常 / 异常
深色主题：正常 / 异常
三个页面同步切换主题：成功 / 失败
重启恢复浅色主题：成功 / 失败
应用输出中的警告或错误：
git diff --stat 输出：
```

暂时不要提交应用代码。验收通过后，我们再分别提交文档和主题代码。

## 14. 完成记录

完成日期：2026-10-07。

验收结果：

```text
CMake 配置：成功
Debug 构建与运行：成功
浅色主题：正常
深色主题：正常
三个页面同步切换主题：成功
重启恢复浅色主题：成功
QML 运行时错误：无
all_qmllint：通过，无警告
CMake QML 注册：无重复项
```

本课实现了 AppTheme 单例和 FeaturePlaceholder 通用组件。开发过程中发现
`onPrimaryColor` 会被 QML 按信号处理器语法解析，已改名为
`textOnPrimaryColor`，并同步修正文档。
