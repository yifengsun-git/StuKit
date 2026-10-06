# 第一课：准备开发环境

## 本课目标

本课只完成开发工具安装和验证，不创建正式的 StuKit 业务代码。

完成后，你应该理解：

- Qt Creator 是编辑和调试工程的 IDE。
- Qt 是应用框架和库。
- 编译器将 C++ 源码转换为 Windows 程序。
- CMake 描述工程由哪些目标和文件组成。
- Kit 是 Qt 版本、编译器、调试器和构建工具的组合。

## 推荐安装内容

通过 Qt 官方在线安装器安装：

- Qt Creator。
- 一个当前稳定的 Qt 6 桌面版本。
- 对应的 64 位编译器组件。
- CMake。
- Ninja。
- Qt Debug Information，可选但推荐。
- Qt Sources，可选，方便查看源码和调试。

对于第一次搭建项目，推荐先选择一种 Windows 编译器，不要同时配置多个
Kit。优先选择 MSVC 64-bit；如果电脑没有可用的 Visual Studio C++ 工具链，
可以选择安装器提供的 MinGW 64-bit。

Android 组件本课暂不安装。Windows 工程稳定后再单独配置 Android SDK、NDK、
JDK 和对应 Qt 套件，可以减少初学阶段的变量。

## MSVC 与 MinGW 如何选择

选择 MSVC 的条件：

- 已安装 Visual Studio 2022 或 Build Tools。
- 安装了“使用 C++ 的桌面开发”工作负载。
- 希望以后使用更多 Windows 原生库。

选择 MinGW 的条件：

- 不想安装 Visual Studio 工具链。
- 希望由 Qt 安装器一次安装完整编译环境。

两者都能完成本项目。选择之后，项目依然使用 CMake，因此以后可以增加另一套
Kit。不要混用不同编译器构建出的第三方二进制库。

## 验证步骤

打开 Qt Creator，进入首选项中的 Kits 页面，确认桌面 Kit 没有红色错误标记，
并记录以下信息：

```text
Qt Creator 版本：
Qt 版本：
Kit 名称：
编译器：
CMake 版本：
调试器：
```

然后使用 Qt Creator 自带的新建项目向导创建一个临时 Qt Quick Application，
要求：

- 构建系统选择 CMake。
- Qt 版本选择 Qt 6。
- 使用 Debug 配置。
- 临时项目放在 StuKit 仓库之外，避免将练习文件混入正式项目。

构建并运行后，确认能够看到示例窗口。随后在一个 QML 文本元素中修改显示文字，
重新运行，确认修改生效。

## 验收清单

- [ ] Qt Creator 能正常启动。
- [ ] 至少有一个可用的 Desktop 64-bit Kit。
- [ ] 临时 Qt Quick 程序能够完成配置和构建。
- [ ] Debug 模式能够运行程序。
- [ ] 修改 QML 后能够看到界面变化。
- [ ] 已记录 Qt、Kit、编译器和 CMake 版本。

## 完成后反馈

请提供记录的六项版本信息，并说明临时程序是否成功运行。如果失败，请提供
Qt Creator“问题”面板中的第一条完整错误，以及“编译输出”中对应的上下文。

环境验收通过后，下一课将创建 StuKit 的正式 CMake 工程和最小 Qt Quick
应用外壳。

## 本项目环境记录

记录日期：2026-10-06。

```text
Qt Creator：18.0.1
Qt：6.10.1
主要 Kit：Desktop Qt 6.10.1 MSVC 2022 64-bit
备用 Kit：Desktop Qt 6.10.1 MinGW 64-bit
CMake：4.3.3
临时程序：使用 MSVC Kit 构建运行成功
调试器：待通过断点验证
```

环境已经满足创建正式工程的条件。本项目现阶段统一使用 MSVC 2022 64-bit
Kit，避免在同一个构建目录中混用 MSVC 和 MinGW。
