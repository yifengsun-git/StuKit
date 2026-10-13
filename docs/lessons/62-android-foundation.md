# 第六十二课：Android 适配基线

## 本课目标

在不复制业务代码的前提下，让现有 Qt Quick 工程具备 Android 构建入口，并先处理
桌面端与 Android 最容易产生差异的应用类型、窗口安全区、返回键、生命周期和文件
选择结果。

## 当前工具链

本项目统一使用 Qt 6.10.3。已安装的 Qt 组件包括 Windows MSVC 2022 64-bit，以及
Android 的 `arm64-v8a`、`armeabi-v7a`、`x86` 和 `x86_64` ABI。首个真机版本只使用
`arm64-v8a`，它覆盖绝大多数现代 Android 手机，也能减少首次构建时间。

Qt 6.10 的 Android 工具链要求如下：

- JDK 17；
- Android SDK Platform 36；
- Android Build Tools 36.0.0；
- Android NDK 27.2.12479018；
- Android 9（API 28）作为最低系统版本。

不要直接把当前 PATH 中的 JDK 25 当作本项目 Android JDK。应在 Qt Creator 的
`编辑 > Preferences > SDKs > Android` 页面中为 Qt Android 单独选择 64 位 JDK 17，
并让 `Set Up SDK` 安装和校验上述 SDK、Build Tools、Platform Tools 与 NDK。

完成设置后应勾选自动创建 Android Kit。项目的首选 Kit 为
`Android Qt 6.10.3 Clang arm64-v8a`；桌面开发仍使用
`Desktop Qt 6.10.3 MSVC2022 64bit`。

## 本次代码适配

### 1. 构建入口

根 `CMakeLists.txt` 已统一要求 Qt 6.10，并移除主程序和 QML 测试对 Qt Widgets 的
依赖。Android 构建会额外设置：

- 包名 `io.github.yifengsungit.stukit`；
- 最低 API 28、目标 API 36；
- 版本名称取 CMake 项目版本，版本代码从 1 开始；
- 网络访问权限，用于天气模块。

包名和版本代码都是 CMake 缓存变量，发布前可以在 Kit 初始配置中覆盖，不需要修改
业务源码。Android 构建不编译当前桌面测试目标；Domain、Application、Infrastructure
和 Presentation 仍会作为主程序依赖接受 Android 编译器检查。

### 2. 应用与生命周期

入口由 `QApplication` 改为 `QGuiApplication`。StuKit 使用纯 Qt Quick，不需要 Widgets
事件循环，这也避免 Android 主程序引入不必要的 Widgets 依赖。

Android 应用从后台回到前台时会重新扫描待办提醒。当前提醒仍是应用内提醒；真正能在
应用被系统挂起或结束时触发的通知，需要后续增加 Android 平台通知适配器和后台调度，
不能仅靠 QML 定时器实现。

### 3. 窗口与触摸

主窗口使用 `SafeArea` 给状态栏、刘海、圆角和底部手势区留出空间；Android 不再应用
桌面窗口的最小尺寸约束。系统返回键按以下顺序工作：

1. 数据分析页返回记账本；
2. 功能模块返回首页；
3. 首页把事件交还系统，由 Android 执行默认返回行为。

按钮和课表格子的按下态会立即改变颜色或阴影。鼠标悬停效果只在桌面启用，因此触屏
不依赖 hover 才能看见操作反馈。

### 4. Android 文件选择

Android 文件选择器通常返回 `content://` URI，而不是 Windows 风格的本地路径。本次
适配允许以下流程消费内容 URI：

- 数据库备份：先生成应用缓存文件，再写入用户选择的内容 URI；
- 数据库恢复：先把内容 URI 复制到应用缓存，再复用原有验证和安全恢复服务；
- 流水 CSV 导出：先生成缓存文件，再复制到目标内容 URI；
- 课表 CSV 导入：直接通过 Qt 文件接口读取内容 URI。

这样 Android 差异被限制在文件边界，记账和课表的领域规则、用例与 SQLite Repository
不需要平台分支。临时中转文件位于应用缓存目录，会由临时文件对象自动清理。

## 首次构建步骤

1. 关闭并重新打开 Qt Creator，使其重新扫描 Qt 6.10.3。
2. 在 Android 设置页完成 JDK 17 和 SDK 自动安装，确认各项检查均为绿色。
3. 用 USB 连接手机，启用开发者选项和 USB 调试，并在手机上允许这台电脑调试。
4. 重新打开项目，在 Kit 选择器中启用
   `Android Qt 6.10.3 Clang arm64-v8a`。
5. 执行一次“重新配置项目”，再构建 Debug APK。
6. 选择已连接的手机运行，依次人工检查首页、记账、分析、课表、天气和待办。

Debug 包用于本地真机调试；以后上架 Google Play 时再创建签名密钥并构建 AAB。签名
文件及密码绝不能提交到 GitHub。

## 本轮验收边界

完成条件：

- Qt 6.10.3 MSVC Debug 全量构建成功；
- QML 静态检查和 Windows 自动测试通过；
- Android arm64 Kit 可以配置并生成 Debug APK；
- 真机上可以启动、导航、写入 SQLite，并通过文件选择器导入或导出数据。

如果 Android SDK/JDK 尚未配置，前三项中的 APK 生成会暂时阻塞，但不影响已经完成的
跨平台代码适配和 Windows 验证。下一轮在工具链就绪后只需处理 Android 编译器暴露的
具体差异，不应重新设计业务模块。
