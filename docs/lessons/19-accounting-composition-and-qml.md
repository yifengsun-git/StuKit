# 第十九课：应用组合根、真实数据库与 QML 接线

## 本课定位

第十八课的 ViewModel 已经通过内存测试。本课计划把现有各层在应用启动时组装起来，
让记账首页第一次读取真实 SQLite 数据：

```text
AccountingHomePage.qml
  -> AccountingOverviewViewModel
  -> GetMonthlyOverview
  -> SqliteTransactionRepository
  -> stukit.sqlite
```

## 1. 本课目标

- 使用 `QStandardPaths` 确定跨平台数据库目录。
- 打开 SQLite 并执行迁移 1、2、3。
- 按正确生命周期创建 Repository、用例和 ViewModel。
- 保存并恢复用户自定义时区，不依赖系统时区。
- 把 ViewModel 注入 QML 根对象并逐层传递。
- 将记账占位页替换为最小可用月度总览页面。
- 正确展示加载成功、空数据和启动失败状态。
- 保持 Windows 与 Android 可复用路径。

## 2. 数据库文件位置

禁止写死：

```text
D:\ComputerProject\qt\StuKit\stukit.sqlite
```

正式路径由：

```cpp
QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
```

获得，再使用 `QDir::mkpath()` 创建目录并拼接 `stukit.sqlite`。这样 Windows 和
Android 会自动得到各自允许写入的应用数据目录。

## 3. 组合根

新增一个仅负责对象组装的类型，例如：

```text
src/app/AccountingModule.h
src/app/AccountingModule.cpp
```

创建顺序：

1. `SqliteDatabase`
2. 三个 SQLite Repository
3. `GetMonthlyOverview` 等 Application 用例
4. `AccountingOverviewViewModel`

销毁顺序必须反过来，确保 ViewModel 和 Repository 先释放，数据库连接最后关闭。

组合根可以依赖所有层，但 Domain、Application、Infrastructure 和 Presentation
都不能反向依赖组合根。

## 4. 自定义时区持久化

使用 `QSettings` 保存用户选择，不读取系统时区作为业务默认值。建议键：

```text
accounting/timeZoneKind       fixed 或 named
accounting/timeZoneId         Asia/Shanghai
accounting/fixedOffsetMinutes 480
```

第一版默认值使用固定 UTC+08:00。读取设置后必须重新验证：

- 地区时区 ID 仍然有效。
- 固定偏移在 -840 到 +840 分钟之间。
- 无效设置回退到 UTC+08:00，并覆盖错误值。

以后设置页面只修改这组设置并通知 ViewModel，不需要改动 Domain 或数据库结构。

## 5. 将 ViewModel 传给 QML

推荐由 `QQmlApplicationEngine::setInitialProperties()` 给 `Main.qml` 提供根属性：

```text
main.cpp
  -> Main.qml required property accountingOverview
  -> AccountingHomePage.viewModel
```

相比在任意位置查找 QML 对象，这种方式依赖方向明确，也能在根对象缺少必要依赖时
立即暴露错误。

ViewModel 必须比 `QQmlApplicationEngine` 和所有使用它的 QML 对象活得更久。

## 6. 最小记账首页

本课只制作读取页面，不在同一课实现新增账单表单。页面包含：

- 上一月、月份标签、下一月。
- 月收入、月支出、月结余三项摘要。
- 当前自定义时区 ID。
- 流水 `ListView`。
- 空月份提示。
- 数据库或加载错误提示与重试按钮。

QML 绑定 `amountText`、`isIncome`、`occurredAt` 和 `note` 等角色，不计算汇总金额，
也不执行 SQL。

## 7. 首次运行没有基础数据

全新数据库没有账户、分类和账单，因此页面首先会显示正确的空状态。这不代表读取
链路失败。

为了继续完成“新增账单”界面，后续还需要账户与分类管理页面。第十九课不向正式
数据库偷偷插入演示数据，避免污染用户数据。

## 8. 错误处理

应用启动需要区分：

- 无法创建应用数据目录。
- SQLite 驱动不可用。
- 数据库无法打开。
- 迁移失败。
- 月度查询失败。
- QML 根对象创建失败。

数据库初始化错误应记录具体诊断信息，同时向页面提供适合用户理解的短提示。不要
把完整 SQL 或本机路径直接显示到界面。

## 9. 测试与验证计划

- 使用临时目录测试组合根能创建并迁移数据库。
- 验证自定义时区设置可以保存和恢复。
- 验证无效时区设置回退到 UTC+08:00。
- 验证 ViewModel 能通过真实 SQLite 查询空月份。
- 构建后运行全部 CTest。
- 启动应用并人工检查记账首页空状态、月份切换和错误区域。
- 检查数据库文件没有生成在源码目录。

预计至少新增一个测试目标，总数变为：

```text
17/17 tests passed
```

## 10. 计划修改的文件

可能新增：

```text
src/app/AccountingModule.h
src/app/AccountingModule.cpp
src/app/UserSettings.h
src/app/UserSettings.cpp
tests/integration/app/AccountingModuleTest.cpp
```

需要修改：

```text
CMakeLists.txt
src/app/main.cpp
src/shell/qml/Main.qml
src/features/accounting/qml/AccountingHomePage.qml
tests/CMakeLists.txt
```

最终文件名会以实现中最小、清晰的职责为准，不为了完全照抄计划制造多余类型。

## 11. 验收清单

- [ ] 数据库位于 `QStandardPaths::AppDataLocation`。
- [ ] 启动时自动执行全部数据库迁移。
- [ ] 依赖创建和销毁顺序安全。
- [ ] 自定义时区通过 `QSettings` 持久化。
- [ ] 默认时区为固定 UTC+08:00，而不是系统时区。
- [ ] ViewModel 通过根属性注入 QML。
- [ ] 记账首页显示月份摘要、流水、空状态和错误状态。
- [ ] QML 不包含 SQL 或月度统计逻辑。
- [ ] 全量测试通过。
- [ ] Windows 应用可正常启动和切换月份。
- [ ] 源码目录没有正式数据库文件。
- [ ] 文档与代码导读同步更新。

## 12. 后续方向

完成第十九课后，记账首页将拥有真实读取链路。下一阶段再建立账户、分类与新增账单
表单，让用户能从空数据库创建基础数据并完成第一笔真实记账。
