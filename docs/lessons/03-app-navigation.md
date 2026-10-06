# 第三课：建立应用导航

## 本课目标

将单个文本窗口扩展为应用外壳，提供首页、记账本和课表三个页面。记账和课表
本课只显示占位内容，不包含任何业务逻辑。

完成后，你应该理解：

- 如何将大 QML 文件拆分为可复用组件。
- QML 组件如何通过 signal 向外发送用户意图。
- Drawer 和 StackLayout 如何组成基础导航。
- 功能目录和 QML 模块是两个不同概念。
- 为什么当前阶段暂不拆分多个 CMake 目标。

相关架构决定见：
[ADR 0001：逐步拆分 CMake 构建目标](../architecture/decisions/0001-build-target-structure.md)。

## 1. 本课完成后的目录

新增三个 QML 文件：

```text
src/
  shell/
    qml/
      Main.qml
      HomePage.qml
  features/
    accounting/
      qml/
        AccountingHomePage.qml
    schedule/
      qml/
        ScheduleHomePage.qml
```

页面按照功能存放，但本课仍由 URI 为 StuKit 的同一个 QML 模块管理。

## 2. 修改 CMake 的 QML 文件列表

在根 CMakeLists.txt 中，保留 `qt_add_executable`，将原来的
`set_source_files_properties` 和 `qt_add_qml_module` 两段替换为：

```cmake
set(STUKIT_QML_FILES
    src/shell/qml/Main.qml
    src/shell/qml/HomePage.qml
    src/features/accounting/qml/AccountingHomePage.qml
    src/features/schedule/qml/ScheduleHomePage.qml
)

set_source_files_properties(
    src/shell/qml/Main.qml
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

set_source_files_properties(
    src/shell/qml/HomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS HomePage.qml
)

set_source_files_properties(
    src/features/accounting/qml/AccountingHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS AccountingHomePage.qml
)

set_source_files_properties(
    src/features/schedule/qml/ScheduleHomePage.qml
    PROPERTIES QT_RESOURCE_ALIAS ScheduleHomePage.qml
)

qt_add_qml_module(StuKit
    URI StuKit
    VERSION 1.0
    QML_FILES
        ${STUKIT_QML_FILES}
)
```

这里没有复制文件。`QT_RESOURCE_ALIAS` 只是在编译后的 QML 模块中，为嵌套
目录里的文件指定稳定名称，因此 Main.qml 可以直接创建 HomePage、
AccountingHomePage 和 ScheduleHomePage。

## 3. 创建首页组件

创建 `src/shell/qml/HomePage.qml`：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    signal accountingRequested()
    signal scheduleRequested()

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("欢迎使用 StuKit")
            font.pixelSize: 28
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("选择一个工具开始使用")
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            Button {
                Layout.preferredWidth: 160
                text: qsTr("记账本")
                onClicked: root.accountingRequested()
            }

            Button {
                Layout.preferredWidth: 160
                text: qsTr("课表")
                onClicked: root.scheduleRequested()
            }
        }
    }
}
```

HomePage 不需要知道外部使用 Drawer、StackLayout 还是其他导航方式。它只通过
signal 表达“用户想打开记账本”或“用户想打开课表”，由外层决定如何响应。

## 4. 创建记账占位页

创建 `src/features/accounting/qml/AccountingHomePage.qml`：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("记账本")
            font.pixelSize: 28
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("记账功能将在后续课程中实现")
        }
    }
}
```

## 5. 创建课表占位页

创建 `src/features/schedule/qml/ScheduleHomePage.qml`：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("课表")
            font.pixelSize: 28
            font.bold: true
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("课表功能将在后续课程中实现")
        }
    }
}
```

## 6. 将 Main.qml 改为应用外壳

将 `src/shell/qml/Main.qml` 替换为：

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window

    property int currentPage: 0

    width: 960
    height: 640
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("StuKit")

    function navigateTo(pageIndex) {
        currentPage = pageIndex
        navigationDrawer.close()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 4

            ToolButton {
                text: qsTr("菜单")
                onClicked: navigationDrawer.open()
            }

            Label {
                Layout.fillWidth: true
                text: {
                    switch (window.currentPage) {
                    case 1:
                        return qsTr("记账本")
                    case 2:
                        return qsTr("课表")
                    default:
                        return qsTr("首页")
                    }
                }
                font.bold: true
            }
        }
    }

    Drawer {
        id: navigationDrawer

        width: Math.min(window.width * 0.75, 280)
        height: window.height

        Pane {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label {
                    text: qsTr("StuKit")
                    font.pixelSize: 24
                    font.bold: true
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("首页")
                    onClicked: window.navigateTo(0)
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("记账本")
                    onClicked: window.navigateTo(1)
                }

                Button {
                    Layout.fillWidth: true
                    text: qsTr("课表")
                    onClicked: window.navigateTo(2)
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: window.currentPage

        HomePage {
            onAccountingRequested: window.navigateTo(1)
            onScheduleRequested: window.navigateTo(2)
        }

        AccountingHomePage {}
        ScheduleHomePage {}
    }
}
```

## 7. 理解本课的导航方式

`currentPage` 保存当前页面编号：

```text
0 = 首页
1 = 记账本
2 = 课表
```

Drawer 提供全局入口，StackLayout 根据 `currentIndex` 显示对应页面。首页按钮
发出 signal，Main.qml 收到 signal 后调用 `navigateTo`。

StackLayout 会保留已经创建的页面状态，适合当前三个一级功能页。以后记账模块
内部出现账单列表、编辑和详情页面时，再在模块内部使用 StackView 管理前进和
返回历史。

## 8. 构建与人工验收

修改 QML 文件列表后，Qt Creator 通常会自动重新运行 CMake。如果没有重新
配置，手动选择“构建 -> 运行 CMake”，然后重新构建。

依次检查：

- 启动时显示首页。
- 首页的记账本按钮能打开记账占位页。
- 首页的课表按钮能打开课表占位页。
- 菜单按钮能打开 Drawer。
- Drawer 中三个按钮都能切换页面并自动关闭 Drawer。
- 顶部标题随当前页面变化。
- 缩放窗口时内容仍然居中，没有 QML 错误输出。

## 9. 常见错误

### Type HomePage unavailable

检查 HomePage.qml 是否在 `STUKIT_QML_FILES` 中，并检查资源别名的大小写。

### HomePage is not a type

确认 CMake 已重新配置，而不只是重新运行旧的可执行文件。

### QML 模块加载失败

先检查 CMake 中的 `URI StuKit` 是否仍然与 main.cpp 中的
`loadFromModule("StuKit", "Main")` 一致。

### 页面编号不正确

检查 StackLayout 中页面顺序是否为首页、记账本、课表，并与 0、1、2 对应。

## 10. 本课验收清单

- [ ] 三个新页面文件位于正确目录。
- [ ] CMake 重新配置和 Debug 构建成功。
- [ ] 首页按钮导航正确。
- [ ] Drawer 导航正确。
- [ ] 页面标题正确变化。
- [ ] 调整窗口大小没有明显布局问题。
- [ ] 应用输出没有 QML 错误。
- [ ] `git status` 中没有构建产物。

## 11. 完成后反馈

请提供：

```text
CMake 配置：成功 / 失败
Debug 构建：成功 / 失败
首页导航：成功 / 失败
Drawer 导航：成功 / 失败
窗口缩放：正常 / 异常
应用输出中的警告或错误：
git diff --stat 输出：
```

不要在本课加入数据库或实际记账功能。验收通过后，再提交导航代码并进入主题
与通用控件设计。

## 12. 完成记录

完成日期：2026-10-07。

验收结果：

```text
CMake 配置：成功
Debug 构建：成功
首页导航：成功
Drawer 导航：成功
页面标题切换：成功
窗口缩放：正常
QML 运行时错误：无
增量构建：成功，ninja: no work to do
构建产物：未进入 Git 状态
```

本课期间发现并修正了两类典型问题：Qt 枚举名称拼写错误，以及 Qt Creator
自动添加 QML 文件后造成的 CMake 重复注册。应用现在具备三个一级页面和统一
导航入口，可以继续建立主题及通用控件。
