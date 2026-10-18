# StuKit

[![CI and Release](https://github.com/yifengsun-git/StuKit/actions/workflows/windows.yml/badge.svg)](https://github.com/yifengsun-git/StuKit/actions/workflows/windows.yml)

StuKit 是一个使用 Qt 6 开发的个人学习与生活工具箱，当前以 Windows 桌面端为主要
发布平台，并保留 Android 适配基础。项目将界面、应用用例、领域模型与数据访问分层
组织，方便在保持现有数据与业务逻辑的前提下持续增加新模块。

## 已有功能

- **记账本**：管理账户和自定义收支分类，新增、编辑、删除与筛选流水，查看账户余额、
  月度汇总和多种趋势图表，并支持 CSV 导出及 SQLite 数据备份与恢复。
- **课表**：管理学期、课程资料和节次时间，以周网格查看课程，按教学周批量排课，
  检查时间冲突，并从 UTF-8 CSV 导入课表。
- **待办**：创建自定义分类的任务，设置截止和提醒时间、优先级及关联课程，支持完成、
  归档、恢复、搜索和完成率统计。
- **Windows 更新**：在应用内检查 GitHub Release、下载并校验安装包，安装前自动备份
  SQLite 数据库，再启动覆盖安装。
- **天气（暂时隐藏）**：和风天气查询、七日预报及离线缓存代码已经保留，目前不在
  应用导航和首页展示，后续可以重新启用。

所有个人数据默认保存在设备本地，不会提交到 Git 仓库。桌面与手机界面共用一套
响应式 QML 组件，能够根据窗口宽度、触摸操作和安全区域自动调整布局。

## 下载与安装

请从 [GitHub Releases](https://github.com/yifengsun-git/StuKit/releases) 下载最新版本：

- Windows x64：推荐下载安装程序，也可以下载便携 ZIP 后运行 `bin/StuKit.exe`。
- Android：目前暂停发布。`v0.1.0` 中的测试 APK 仅用于早期个人设备试用，不代表
  当前 Windows 版本的完整功能和稳定性。

从 `v0.2.0` 开始，Windows 版支持在应用内手动检查更新。已经安装 `v0.1.0` 的用户
需要手动安装一次 `v0.2.0`，后续版本即可使用应用内更新。Windows 安装程序暂未进行
商业代码签名，因此操作系统可能显示未知发布者或安全提醒；请只从本仓库的 Releases
页面下载。

Windows 安装程序允许选择应用根目录。例如选择 `D:\stu` 后，程序安装在
`D:\stu\StuKit`，个人数据库、设置、缓存与备份保存在 `D:\stu\data`。升级和常规卸载
默认不会删除 `data`。从旧版升级时，程序会把 `%APPDATA%\StuKit\StuKit` 中的现有数据
安全复制到新目录，原目录保留作为迁移备份。便携 ZIP 未包含安装器生成的存储标记，
因此仍使用系统标准的应用数据目录。

## 技术栈

- Qt 6.10、Qt Quick、QML、Qt Charts
- C++20、CMake
- SQLite、Qt SQL
- Clean Architecture、MVVM、模块化单体
- Qt Test、GitHub Actions

## 本地构建

推荐使用 Qt Creator 18、Qt 6.10.3 和 CMake。Windows 桌面端使用 MSVC 2022 x64；
Android 端使用 Qt Android arm64-v8a Kit、JDK 17，以及 Qt 对应版本要求的 Android
SDK 与 NDK。打开仓库根目录的 `CMakeLists.txt`，选择对应 Kit 后即可配置、构建和运行。

## 许可证

本项目采用 [MIT License](LICENSE)。
