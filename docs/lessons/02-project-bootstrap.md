# 第二课：创建正式工程骨架

## 本课目标

由学习者亲手创建 StuKit 的第一个可运行版本。本课只建立应用入口和一个最小
QML 窗口，不添加记账、数据库、导航或主题功能。

完成后，你应该理解：

- CMakeLists.txt 如何描述一个 Qt 工程。
- C++ 程序如何启动 QML 引擎。
- QML 文件如何描述窗口和控件。
- Qt Creator 如何选择 Kit、配置、构建、运行和调试。

## 1. 本项目统一选择的 Kit

本阶段统一使用：

```text
Desktop Qt 6.10.1 MSVC 2022 64-bit
```

MinGW Kit 保留备用，但不要让 MSVC 和 MinGW 共用同一个构建目录。不同
编译器生成的目标文件和第三方二进制库通常不能直接混用。

Qt Creator、VS Code 和 Visual Studio 是编辑器或 IDE；MSVC 2022 64-bit
才是本阶段实际使用的 C++ 编译器。

## 2. 创建目录和文件

在仓库根目录中亲手创建以下结构：

```text
StuKit/
  CMakeLists.txt
  .gitignore
  src/
    app/
      main.cpp
    shell/
      qml/
        Main.qml
```

可以先用 VS Code 创建这些文件，然后在 Qt Creator 中打开根目录的
CMakeLists.txt。使用哪个编辑器创建文本文件不会改变项目技术栈。

## 3. 根 CMakeLists.txt

写入以下内容：

```cmake
cmake_minimum_required(VERSION 3.21)

project(StuKit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(StuKit
    src/app/main.cpp
)

set_source_files_properties(
    src/shell/qml/Main.qml
    PROPERTIES QT_RESOURCE_ALIAS Main.qml
)

qt_add_qml_module(StuKit
    URI StuKit
    VERSION 1.0
    QML_FILES
        src/shell/qml/Main.qml
)

target_link_libraries(StuKit
    PRIVATE Qt6::Quick
)
```

逐段理解：

- `cmake_minimum_required` 指定构建该项目所需的最低 CMake 版本。
- `project` 定义项目名称、版本和使用的语言。
- `CMAKE_CXX_STANDARD` 要求编译器使用 C++20。
- `find_package` 查找 Qt 6 的 Quick 模块。
- `qt_add_executable` 创建名为 StuKit 的可执行程序。
- `QT_RESOURCE_ALIAS` 让嵌套目录中的 Main.qml 在 QML 模块内仍叫 Main.qml。
- `qt_add_qml_module` 创建 URI 为 StuKit 的 QML 模块。
- `target_link_libraries` 将 Qt Quick 链接到应用目标。

`find_package` 要求 Qt 6.8 或更高版本，不是要求你另外安装 Qt 6.8。当前的
Qt 6.10.1 满足这一条件。

## 4. C++ 应用入口

在 `src/app/main.cpp` 写入：

```cpp
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("StuKit");
    QCoreApplication::setApplicationName("StuKit");

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("StuKit", "Main");

    return app.exec();
}
```

关键概念：

- `QGuiApplication` 管理 GUI 应用的生命周期和事件循环。
- `QQmlApplicationEngine` 加载并运行 QML。
- 组织名和应用名将来会用于设置和应用数据路径。
- QML 根对象创建失败时，程序以错误状态退出。
- `loadFromModule` 中的 `StuKit` 必须和 CMake 中的 `URI StuKit` 一致。
- `app.exec()` 开始事件循环，窗口才能持续响应用户操作。

## 5. 第一个 QML 窗口

在 `src/shell/qml/Main.qml` 写入：

```qml
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 960
    height: 640
    minimumWidth: 640
    minimumHeight: 480
    visible: true
    title: qsTr("StuKit")

    Label {
        anchors.centerIn: parent
        text: qsTr("StuKit 工程已成功启动")
        font.pixelSize: 24
    }
}
```

关键概念：

- `ApplicationWindow` 是应用的顶层窗口。
- QML 属性使用 `名称: 值` 的声明式写法。
- `anchors.centerIn` 将 Label 放在父对象中央。
- `qsTr` 为将来的界面翻译保留入口。
- 窗口大小只是 Windows 初始大小，后续界面不能依赖固定尺寸。

## 6. 创建 .gitignore

在仓库根目录的 `.gitignore` 写入：

```gitignore
# CMake and build output
build/
build-*/
cmake-build-*/
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
compile_commands.json
.ninja_deps
.ninja_log

# Qt Creator local settings
*.user
*.user.*

# Generated binaries and debug files
*.exe
*.dll
*.obj
*.pdb
```

源码和文档进入 Git，构建产物及个人 IDE 配置不进入 Git。

## 7. 在 Qt Creator 中构建

1. 选择“文件 -> 打开文件或项目”。
2. 打开仓库根目录的 `CMakeLists.txt`。
3. 只勾选 Desktop Qt 6.10.1 MSVC 2022 64-bit Kit。
4. 构建类型选择 Debug。
5. 将构建目录放在源码目录之外，或使用会被 `.gitignore` 忽略的 build 目录。
6. 点击“配置项目”。
7. 先构建，再运行。

成功时应看到标题为 StuKit 的窗口，以及居中的“StuKit 工程已成功启动”。

## 8. 验证调试器

在 `main.cpp` 的下面一行设置断点：

```cpp
QGuiApplication app(argc, argv);
```

使用“开始调试”而不是普通运行。如果程序在这一行暂停，并且可以继续运行，
调试器配置有效。

如果提示没有调试器，不要改代码。记录 Kit 中 Debugger 字段和完整错误信息，
下一步再检查 Windows SDK/CDB 配置。

## 9. 本课验收清单

- [ ] Qt Creator 使用 MSVC 2022 64-bit Kit 成功配置项目。
- [ ] CMake 配置无错误。
- [ ] Debug 构建成功。
- [ ] StuKit 窗口成功显示。
- [ ] 窗口中央文字正确显示。
- [ ] 调试模式能在 main.cpp 断点处暂停，或已记录调试器错误。
- [ ] `git status` 中没有 build 目录或生成的二进制文件。
- [ ] 能用自己的话解释 CMake、main.cpp 和 Main.qml 各自的作用。

## 10. 完成后反馈

请提供：

```text
CMake 配置：成功 / 失败
Debug 构建：成功 / 失败
窗口运行：成功 / 失败
断点调试：成功 / 失败
git status 输出：
```

如果出现错误，请同时提供第一条完整错误及前后相关输出。验收通过后，下一课
将拆分 CMake 子模块，并建立应用导航和功能模块占位页。

## 完成记录

完成日期：2026-10-06。

验收结果：

```text
CMake 配置：成功
Debug 构建：成功
窗口运行：成功
QML 调试：成功启用
程序退出：正常，退出码为 0
增量构建：成功，ninja: no work to do
构建产物：已被 .gitignore 排除
```

正式工程已经包含 CMakeLists.txt、main.cpp、Main.qml 和 .gitignore，目录结构
符合本课要求。下一步先建立 Git 检查点，再继续扩展应用外壳。
