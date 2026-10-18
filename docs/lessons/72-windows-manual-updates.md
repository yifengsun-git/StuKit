# 第 72 课：Windows 手动更新

## 本课目标

让已经安装的 StuKit 可以在应用内检查 GitHub Release，下载新的 Windows
安装包并启动覆盖安装，同时保留 AppData 中的个人数据。

Android 更新在这一阶段暂停。Android 源码和构建配置仍保留，但不会阻塞
Windows 的构建与发布。

## 更新流程

```text
用户点击检查更新
  -> 请求 GitHub 最新 Release
  -> 比较当前版本和 Release 标签
  -> 查找 Windows setup.exe
  -> 下载到应用缓存目录
  -> 校验 GitHub 提供的 SHA-256
  -> 备份 SQLite 数据库
  -> 启动 Inno Setup 安装器
  -> 退出 StuKit
```

更新器不会覆盖正在运行的程序文件。它只负责下载并启动独立的安装器，
由 Inno Setup 使用相同的 `AppId` 和安装目录完成覆盖安装。

## 代码分层

- `features/update/domain` 保存 `UpdateInfo`，只描述更新数据。
- `features/update/application` 声明检查、下载、安装和更新前准备接口。
- `features/update/infrastructure/github` 访问并解析 GitHub Release。
- `features/update/infrastructure/platform` 启动 Windows 安装器。
- `features/update/presentation` 向 QML 暴露状态和操作。
- `app/UpdateModule` 负责组装更新模块。
- `app/AccountingUpdatePreparation` 在安装前创建 SQLite 备份。

这样以后恢复 Android 开发时，只需要增加 Android 平台安装适配器，不需要
重写版本检查和下载逻辑。

## 版本来源

版本号只在根目录 `CMakeLists.txt` 中定义：

```cmake
project(StuKit VERSION 0.2.0 LANGUAGES CXX)
```

CMake 将它传给应用，应用再通过 `QCoreApplication::applicationVersion()` 提供
给更新模块。发布标签继续使用 `v0.2.0` 形式。

## 数据为什么不会丢失

当前安装器让用户选择一个根目录。若选择 `D:\stu`，程序安装到
`D:\stu\StuKit`，数据库、设置、缓存和备份统一保存在 `D:\stu\data`。
两个目录彼此独立，所以覆盖安装只替换程序文件，卸载也不会自动删除
`data`。旧版本位于 Qt `AppDataLocation` 的数据会在首次启动时复制到新目录，
原文件仍会保留，迁移不会覆盖新目录中已经存在的文件。

点击“安装并退出”前还会在数据目录下创建：

```text
backups/before-update-to-版本号-时间.sqlite
```

只有备份成功后才会启动安装器。如果备份失败，更新会停止并显示错误。

## GitHub Actions 改动

Windows 发布产物新增 `checksums.sha256`。Android job 暂时只允许在手动运行
工作流时勾选 `build_android` 执行，Git 标签发布不再等待 Android。

下一次发布至少包含：

```text
StuKit-0.2.0-windows-x64-setup.exe
StuKit-windows-x64.zip
checksums.sha256
```

GitHub [官方 Release API](https://docs.github.com/en/rest/releases/releases)
的资源对象包含 `digest` 字段，格式为
`sha256:<64 位十六进制摘要>`。更新器只接受这个 SHA-256 摘要；缺少摘要或
下载结果不匹配时都会禁止安装。`checksums.sha256` 同时保留，便于用户手动
核对发布文件。

## 验收方法

1. 构建并运行当前 `0.2.0`，更新页应显示当前版本。
2. 正式发布 `v0.2.0` 前，GitHub 最新版本仍为 `v0.1.0`，因此程序应提示
   当前已是最新版，而不是把旧版本当作更新。
3. 本地集成测试用模拟 Release `v0.3.0` 验证新版本识别，不需要反复修改
   项目版本号或访问 GitHub。
4. 下载时应显示进度，取消后应用保持可用。
5. 下载完成后点击安装，确认 `backups` 中生成数据库备份。
6. 覆盖安装后启动新版本，确认账本、课表和待办数据仍然存在。

不要为了测试重复移动已经发布的正式标签。完整升级测试应使用新的测试
Release 或单独的测试仓库。

## 本课自动验收结果

- 项目版本已从 `0.1.0` 提升为 `0.2.0`。
- 使用本地模拟 Release 验证了 `v0.3.0` 识别、安装包下载和 SHA-256 校验。
- 验证了错误摘要不会留下可安装文件。
- 验证了取消下载后立即重试不会让旧请求破坏新请求。
- 验证了更新前备份是可读取、版本受支持的 SQLite 数据库。
- Debug 全量测试共 `62` 项，全部通过。
- Release 构建、Qt 运行库部署和隐藏窗口启动检查均通过。

本机没有安装 Inno Setup 编译器，因此本地没有生成最终 `setup.exe`；GitHub
Actions 会在 Windows runner 上自动安装 Inno Setup 并完成这一环节。

## 第一次过渡升级

已经发布的 `v0.1.0` 本身没有更新器，所以从 `v0.1.0` 到第一个带更新器的
`v0.2.0` 仍需手动下载安装一次。此后只要新版本沿用同一 `AppId`、安装目录和
Release 文件命名，便可以从应用内完成后续更新。
