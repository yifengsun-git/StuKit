# 第二十九课：Windows Release 部署基础

## 本课定位

第二十八课完成了记账数据的备份、恢复和 CSV 导出。开发目录中的程序能够运行，
并不代表复制一个 `StuKit.exe` 就能交给其他电脑使用；Qt 应用还需要 Qt 动态库、
平台插件、QML 模块和 SQLite 驱动。本课建立可重复的 Windows Release 部署流程。

## 1. 本课目标

- 使用 MSVC2022 64-bit Kit 生成独立 Release 构建。
- 通过 CMake 安装规则生成结构明确的暂存目录。
- 使用与 Kit 匹配的 `windeployqt` 收集 Qt 运行时依赖。
- 确认 QML、Windows 平台插件和 SQLite 驱动都已部署。
- 在不借用构建目录 PATH 的环境中启动部署后的程序。
- 写下可重复执行的命令和常见错误排查方式。

## 2. 为什么不能只复制 EXE

`StuKit.exe` 依赖 Qt Core、Quick、QML、Controls、SQL 等运行库，还需要：

- `platforms/qwindows.dll`：Windows 窗口平台插件。
- `sqldrivers/qsqlite.dll`：SQLite 驱动。
- QML 模块与对应插件。
- MSVC 运行库（具体处理方式在部署验证时确认）。

缺少这些文件时，常见现象是程序无窗口退出、提示找不到 DLL、无法加载 QML 组件，
或数据库驱动不可用。

## 3. 计划中的输出结构

```text
dist/windows/
  StuKit.exe
  Qt6*.dll
  platforms/
  sqldrivers/
  qml/
  ...
```

`dist/` 是生成产物，不提交到 Git。正式源代码、部署脚本和说明文档需要提交。

## 4. 执行计划

1. 检查并补充应用安装规则与版本元数据。
2. 创建全新的 MSVC Release 构建目录。
3. 构建并安装到暂存目录。
4. 调用当前 Qt Kit 自带的 `windeployqt`。
5. 检查关键插件和 QML 模块。
6. 在隔离环境进行启动冒烟测试。
7. 记录人工验证步骤和失败排查方法。

第一版先形成可复制的文件夹，不急于制作安装向导。文件夹部署验证稳定后，再决定
使用 ZIP、Qt Installer Framework、MSIX 或其他安装形式。

## 5. 验收清单

- [ ] Release 构建可以从全新目录完成。
- [ ] CMake 安装目录包含 `StuKit.exe`。
- [ ] `windeployqt` 收集所有必需依赖。
- [ ] 部署目录包含 Windows 平台插件和 SQLite 驱动。
- [ ] 部署后的程序能在隔离环境启动并创建数据库。
- [ ] 自动测试和 QML lint 继续通过。
- [ ] 部署目录不会提交到 Git。
- [ ] README 与代码导读同步更新。
