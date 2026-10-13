# StuKit CI/CD 与 Windows 发布说明

## 1. 当前目标

当前流水线只承担 Windows x64 桌面版的持续集成和可重复交付：

- Qt 版本固定为 6.10.3。
- 编译器固定为 MSVC 2022 x64。
- 每次推送到 `main`、向 `main` 提交 Pull Request 或手动触发时，自动配置、编译、
  运行测试、部署并生成 ZIP。
- 推送与 `CMakeLists.txt` 项目版本一致的 `v*` 标签时，在测试通过后自动创建
  GitHub Release。

当前阶段不生成 Windows 安装程序、不做代码签名，也不在 CI 中构建 Android APK。
这些内容在界面稳定后单独增加，避免同时维护过多变量。

## 2. 文件与职责

- `.github/workflows/windows.yml`：Windows 构建、测试、打包和 Release 发布。
- `.github/dependabot.yml`：每周检查 GitHub Actions 的新版本。
- `CMakeLists.txt`：定义标准安装目录，并调用 Qt 的 QML 应用部署 API。

Qt 部署脚本会在执行 `cmake --install` 时收集应用需要的 Qt DLL、QML 模块、平台
插件和 MSVC 运行时。数据库文件不属于程序包；用户数据仍在应用数据目录中创建，
不会进入 GitHub Artifact 或 Release。

## 3. 流水线过程

```text
checkout
   ↓
缓存或下载 Qt 6.10.3 + Qt Charts
   ↓
CMake Release 配置（MSVC 2022 x64）
   ↓
编译全部目标
   ↓
CTest 全量测试
   ↓
cmake --install 收集运行依赖
   ↓
offscreen 启动 5 秒冒烟测试
   ↓
StuKit-windows-x64.zip Artifact
   ↓ 仅 v* 标签
GitHub Release
```

任意一步失败都会停止后续步骤，因此测试失败的提交不会生成正式 Release。

## 4. 触发规则

| 操作 | 构建 | 测试 | ZIP Artifact | GitHub Release |
| --- | --- | --- | --- | --- |
| push 到 `main` | 是 | 是 | 是，保留 14 天 | 否 |
| Pull Request 到 `main` | 是 | 是 | 是，保留 14 天 | 否 |
| Actions 页面手动运行 | 是 | 是 | 是，保留 14 天 | 否 |
| push `v*` 标签 | 是 | 是 | 是 | 是 |

并发执行同一分支的新旧流水线时，旧任务会被取消，减少无效构建时间。

## 5. 首次启用

将本次文件提交并推送到 GitHub 后：

1. 打开仓库的 **Actions** 页面。
2. 选择 **Windows CI and Release**。
3. 点击 **Run workflow**，先手动执行一次。
4. 确认 Build、CTest、部署冒烟测试和 Artifact 上传全部成功。
5. 打开仓库 **Settings → Branches/Rulesets**，为 `main` 增加保护规则，并要求
   `Build, test, and package Windows` 检查通过后才能合并。

流水线不需要个人访问令牌、Qt 账号或仓库 Secret。发布任务使用 GitHub 为当前任务
临时创建的 `GITHUB_TOKEN`，并且只有发布 Job 获得 `contents: write` 权限。若发布时
出现 403，再检查 **Settings → Actions → General → Workflow permissions** 是否被
仓库或组织策略强制限制。

## 6. 创建版本

发布前应先完成 UI/功能验收，并保持工作区干净。以 `0.1.0` 为例：

1. 将 `CMakeLists.txt` 中的项目版本设为 `0.1.0`。
2. 提交版本相关改动并推送 `main`。
3. 等待 `main` 的 CI 通过。
4. 创建并推送标签：

```powershell
git tag -a v0.1.0 -m "StuKit 0.1.0"
git push origin v0.1.0
```

流水线会验证标签必须等于 `v` 加项目版本。比如项目仍为 `0.1.0` 时，`v0.2.0`
会被拒绝，避免压缩包版本与源代码版本不一致。

成功后，仓库的 **Releases** 页面会出现自动生成的发布说明和
`StuKit-windows-x64.zip`。用户解压后从 `bin/StuKit.exe` 启动。

## 7. 当前边界与后续增强

- ZIP 当前没有数字签名，Windows SmartScreen 可能提示未知发布者。
- 还没有生成安装器、开始菜单快捷方式或自动更新程序。
- CI 暂不连接真实天气服务，网络行为通过现有测试替身验证。
- Android CI、签名 APK/AAB 和 Play 分发在响应式 UI 验收后建设。
- 后续可加入 SBOM、Release 校验文件和 Windows 代码签名。

这些限制不会影响当前流水线对编译、自动测试和可运行目录完整性的验证。
