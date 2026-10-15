# StuKit CI/CD 与发布说明

## 发布目标

GitHub Actions 使用 Qt 6.10.3 自动验证 Windows 与 Android 两个平台。普通分支和
Pull Request 运行构建检查；与 `CMakeLists.txt` 版本一致的 `v*` 标签会创建 GitHub
Release，并附带以下文件：

- Windows x64 安装程序；
- Windows x64 便携 ZIP；
- Android arm64-v8a 可安装测试 APK。

本地 SQLite 数据库、备份、天气服务凭据和构建目录都不会进入发布包或 Git 历史。

## Windows 流程

Windows Job 使用 MSVC 2022 x64，安装 Qt Quick、Qt SQL、Qt Charts 等运行依赖，
完成 Release 编译、全量 CTest、部署目录生成和无界面冒烟测试。验证通过后生成便携
ZIP，并使用 Inno Setup 生成当前用户范围的安装程序；安装过程不需要管理员权限。

## Android 流程

Android Job 使用 JDK 17、Android SDK 36、Build Tools 36.0.0、NDK 27.2.12479018
和 Qt Android arm64-v8a Kit。CI 通过 Debug 构建生成可直接安装的测试 APK。

测试 APK 的签名由每次 CI 的临时调试密钥生成，适合个人设备验证，但不同工作流生成
的 APK 不能保证直接覆盖升级。正式上架前，应离线生成并备份长期发布密钥，将加密后的
签名材料放入 GitHub Actions Secrets，然后改为生成签名 Release APK 或 AAB。私钥、
密码和和风天气 API Key 都不能提交到仓库。

## 触发方式

| 操作 | Windows 构建与测试 | Android 构建 | GitHub Release |
| --- | --- | --- | --- |
| push 到 `main` | 是 | 是 | 否 |
| Pull Request 到 `main` | 是 | 是 | 否 |
| Actions 手动运行 | 是 | 是 | 否 |
| push `v*` 标签 | 是 | 是 | 是 |

## 创建版本

发布前先修改并提交 `CMakeLists.txt` 中的项目版本。以首个版本为例：

```powershell
git tag -a v0.1.0 -m "StuKit 0.1.0"
git push origin main
git push origin v0.1.0
```

流水线会拒绝与项目版本不一致的标签。两个平台均成功构建后才会创建 Release；任一
平台失败都不会发布不完整版本。

## 当前限制

- Windows 安装程序未购买商业代码签名，SmartScreen 可能显示未知发布者。
- Android 当前只发布 arm64-v8a 测试 APK，未生成应用商店使用的 AAB。
- 天气功能代码保留但 UI 暂时隐藏，发布构建不需要和风天气密钥。
- 项目没有自动更新器，升级包由用户从 Releases 页面手动下载安装。
