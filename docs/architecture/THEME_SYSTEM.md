# StuKit 主题系统

## 目标

主题只负责视觉表现，不改变记账、课表、待办等模块的业务流程和数据。
业务页面统一读取 `AppTheme` 令牌，不能自行判断当前主题 ID，也不要直接写死颜色。

当前版本提供三套内置主题：

- `default`：StuKit 默认主题；
- `dark`：深夜专注主题；
- `cat-warm`：肥猫勿喂暖色主题。

用户可以从“外观与主题”页面即时切换。选择结果保存在应用数据目录的
`settings.ini` 中，键名为 `appearance/themeId`。

第三方主题使用 `.stukit-theme` 扩展名。应用兼容早期的单个 UTF-8 JSON 文件，也支持
包含 `theme.json`、预览图和受限图片资源的 ZIP 主题包。用户在外观页面选择文件后，
应用只会解析并验证内容，同时展示组件预览；只有用户确认安装后，主题才会保存到应用
数据目录的 `themes/<themeId>/` 并启用。取消预览不会产生主题文件。

## 组成部分

- `AppearanceSettingsViewModel`：提供主题目录、当前主题和持久化能力；
- `AppTheme.qml`：接收主题定义并向全部 QML 页面暴露设计令牌；
- `AppearancePage.qml`：显示主题预览并处理用户选择；
- `ThemePreviewPanel.qml`：使用指定主题令牌绘制隔离的组件预览，不改变当前主题；
- `Main.qml`：在应用启动和主题选择变化时应用主题。

主题定义由应用层传入 `AppTheme`，因此后续从 JSON 主题包读取相同字段时，
不需要修改业务页面。

## 主题定义字段

每套主题至少应提供以下元数据：

| 字段 | 含义 |
| --- | --- |
| `themeId` | 全局唯一且稳定的主题 ID |
| `schemaVersion` | 当前固定为 `1` |
| `name` | 用户可见名称 |
| `description` | 主题说明 |
| `author` | 作者 |
| `version` | 主题版本 |
| `dark` | 是否为深色主题 |
| `builtIn` | 是否为内置主题 |
| `previewImage` | 可选；ZIP 包内的 PNG、JPEG 或 WebP 预览图相对路径 |
| `backgroundImage` | 可选；全局装饰背景的 PNG、JPEG 或 WebP 相对路径 |
| `backgroundImageOpacity` | 可选；装饰背景透明度，范围 `0～0.18` |
| `backgroundImageFillMode` | 可选；`cover`、`contain` 或 `tile` |
| `navigationLogo` | 可选；导航品牌图，可使用安全 SVG 或位图 |
| `icons` | 可选；固定导航槽位到 SVG/位图路径的映射 |

颜色令牌：

- `backgroundColor`
- `surfaceColor`
- `surfaceRaisedColor`
- `surfaceVariantColor`
- `surfaceHoverColor`
- `selectionColor`
- `primaryColor`
- `primaryHoverColor`
- `primaryPressedColor`
- `textPrimaryColor`
- `textSecondaryColor`
- `textOnPrimaryColor`
- `borderColor`
- `borderStrongColor`
- `errorColor`
- `successColor`
- `warningColor`

尺寸令牌：

- `radiusSmall`
- `radiusMedium`
- `radiusLarge`
- `navigationRailWidth`

间距、字号、触摸目标和响应式断点目前属于应用级规范，不允许单套主题修改，
以避免第三方主题破坏桌面端和移动端布局。经过兼容性验证后再逐步开放。

## 添加内置主题

1. 在 `AppearanceSettingsViewModel.cpp` 的 `builtInThemes()` 中加入主题定义；
2. 为所有必需令牌提供值，不能依赖业务页面回退；
3. 检查浅色和深色文字对比度；
4. 运行 `app.appearance_settings` 与 `app.shell_qml` 测试；
5. 分别检查首页、记账本、课表、待办和数据分析页面；
6. 在 360 像素窄窗口和普通桌面窗口中各检查一次。

## 制作第三方主题

复制 `docs/templates/example.stukit-theme`，修改元数据和令牌后即可在外观页面导入。
若 `themeId` 已存在，确认页会明确提示这是一次主题更新；确认后使用规范化后的新定义
替换旧定义。内置主题仍然不能被第三方文件覆盖。
主题 ID 必须由 2～64 个小写字母、数字、点、下划线或连字符组成，并且不能与内置
主题重复。颜色仅接受 `#RRGGBB` 或 `#RRGGBBAA` 格式。

当前导入器还会限制：

- 旧版 JSON 文件不能为空且不能超过 256 KB；资源包遵循下方单独限制；
- `schemaVersion` 必须为 `1`；
- 所有颜色令牌都必须存在；
- 圆角满足 `0 ≤ small ≤ medium ≤ large ≤ 32`；
- 导航栏宽度必须在 180～320 之间；
- 名称、说明、作者和版本不能为空且不能超过规定长度；
- 第三方主题不能覆盖或删除内置主题。

### 带预览图的资源包

资源主题包采用标准 ZIP 格式，但最终文件扩展名必须改为 `.stukit-theme`：

```text
my-theme.stukit-theme
├── theme.json
├── preview.png
├── LICENSE.txt          # 可选
└── assets/              # 可选
    ├── background.webp
    └── decoration.svg
```

`theme.json` 位于压缩包根目录。需要自定义预览图时加入：

```json
"previewImage": "preview.png"
```

如果根目录存在 `preview.png`、`preview.jpg`、`preview.jpeg` 或 `preview.webp`，即使没有
显式填写 `previewImage`，应用也会自动使用它。

资源主题可以使用受控界面槽位：

```json
{
  "backgroundImage": "assets/background.webp",
  "backgroundImageOpacity": 0.08,
  "backgroundImageFillMode": "cover",
  "navigationLogo": "assets/logo.svg",
  "icons": {
    "home": "assets/icons/home.svg",
    "accounting": "assets/icons/accounting.svg",
    "schedule": "assets/icons/schedule.svg",
    "tasks": "assets/icons/tasks.svg",
    "appearance": "assets/icons/appearance.svg"
  }
}
```

`icons` 只允许 `home`、`accounting`、`schedule`、`tasks`、`weather`、`update` 和
`appearance` 七个槽位。主题只能替换图像，不能增加、删除导航项或改变点击行为。

资源包安全限制：

- 压缩包不超过 5 MB，最多 64 个条目；
- 解压后总大小不超过 10 MB，单个资源不超过 4 MB；
- 只允许根目录的 `theme.json`、预览图、许可证，以及 `assets/` 下的图片和 SVG；
- 拒绝绝对路径、`..`、反斜杠路径、重复路径、加密文件及不支持的压缩算法；
- 不允许 QML、JavaScript、可执行文件或动态库；
- 位图内容必须与扩展名匹配，最大尺寸为 4096×4096、最大 1600 万像素；
- SVG 不超过 512 KB 和 2000 个元素，拒绝脚本、事件处理器、DTD、外部链接及嵌入数据；
- 先写入临时目录，完整成功后再替换原主题，避免更新中断破坏已安装主题。

## 业务页面开发规则

- 颜色、圆角和共享尺寸只能来自 `AppTheme`；
- 优先使用 `AppButton`、`AppTextField`、`AppDialog` 等共享组件；
- 不通过 `AppTheme.themeId` 编写主题专属业务分支；
- 主题不得隐藏保存、删除、返回等必要操作；
- 收入、支出、警告和错误状态必须保持可辨识，不应只依赖颜色表达；
- 新增令牌时应提供默认值，并同步更新所有内置主题和测试。

## 导出与备份

外观页面可以把已安装的第三方主题重新导出为 `.stukit-theme`。导出器只读取该主题的
`theme.json`、预览图、许可证和 `assets/` 中允许的资源，并重新生成标准 ZIP 包；不会
把临时文件、其他主题或应用数据带入备份。导出目标不能位于应用自己的主题数据目录。

内置主题不能直接导出，因为 `default`、`dark` 和 `cat-warm` 是保留 ID，导出的文件也
无法作为第三方主题重新导入。后续若支持“复制为新主题”，必须先要求用户填写新的
`themeId`、名称、作者和版本。

## 开发者诊断

每套主题都可以打开“主题诊断”，检查：

- 页面正文、卡片正文、次要文字和主要按钮文字的 WCAG 对比度；
- 错误色、成功色在卡片表面的可辨识度；
- 预览图、装饰背景、品牌图和每个导航图标是否成功解析；
- 主题名称、作者、版本和资源类型摘要。

正文对比度建议至少 `4.5:1`，状态和非文字视觉元素建议至少 `3:1`。对比度警告不会阻止
主题使用，资源缺失则视为需要修复的错误。

## 后续边界

无论资源能力如何扩展，都不允许主题执行 QML 或 JavaScript，以免第三方主题读取本地
数据或改变业务行为。后续开放字体前仍需增加字体授权和 StuKit 兼容版本检查；主题损坏
时必须自动回退到 `default`。

用户安装的主题应放在应用数据目录的 `themes/<themeId>/` 中，不能写入安装目录，
从而保证应用更新后主题和业务数据仍然保留。
