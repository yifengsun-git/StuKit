# 第三十八课：课表 Presentation 与周视图模型

## 本课成果

本课建立了课表 Application 与未来 QML 页面之间的状态桥梁。现在 C++ Presentation
已经能够选择学期、计算教学周、切换周次，并把指定周课程转换为 QML 可读取的模型。

新增内容：

- `GetAcademicTerms`：读取并稳定排序全部学期。
- `WeeklyScheduleListModel`：向 QML 暴露周课表角色。
- `ScheduleViewModel`：管理学期、教学周、课程列表、空状态和错误状态。
- 两组独立 Qt Test：验证学期查询和完整 Presentation 行为。

## 1. 为什么补充 GetAcademicTerms

`ScheduleViewModel` 需要学期列表，但 Presentation 不应直接查询 Repository。直接查询
会让界面层重新承担存储编排，也会使将来的学期排序规则散落在页面代码中。

因此新增了很小的 `GetAcademicTerms` Application 用例。它将活动学期排在已归档学期
之前，同一状态下按开始日期倒序排列，并把 Repository 失败转换为稳定错误类型。

## 2. WeeklyScheduleListModel

`WeeklyScheduleListModel` 继承 `QAbstractListModel`，将 Application 返回的
`WeeklyScheduleItem` 暴露为以下角色：

- `courseId`、`meetingId`。
- `courseName`、`teacher`、`location`、`color`。
- `weekday`、`weekdayText`。
- `startPeriod`、`endPeriod`、`periodText`。

例如星期三转换为“周三”，第 3～4 节转换为“第 3～4 节”。模型不查询数据库，也不
判断单双周；这些工作已经由 Application 完成。

## 3. ScheduleViewModel

`ScheduleViewModel` 通过构造函数接收 `GetAcademicTerms` 和 `GetWeeklySchedule`，并
提供以下页面状态：

- 学期选项、当前学期 UUID、名称和总周数。
- 当前选择周、周标签和这一周的实际日期范围。
- 是否可以前往上一周或下一周。
- 当前日期是否位于所选学期、是否正在显示当前教学周。
- 课程模型、空状态、同步加载状态和用户可读错误。

首次加载时的选择顺序为：保留原选择、使用外部传入的首选学期、选择包含今天的活动
学期、选择最近的活动学期，最后才选择已归档学期。当前日期不在学期范围时默认显示
第 1 周，不会生成 0 或超过总周数的查询。

## 4. 周次与日期计算

教学周只依赖学期开始日期和注入的“今天”：

```text
教学周 = 开始日期到今天的天数 / 7 + 1
```

只有结果位于 `1..totalWeeks` 时，“返回当前周”才可用。把 `today` 通过构造函数传入，
既避免依赖设备时钟的测试不稳定，也为将来的日期模拟和 Android 复用留下空间。

## 5. 错误与旧数据清理

- 学期查询失败：清空学期、选择和课程，显示“无法读取学期数据”。
- 周课表查询失败：保留学期和周次，清空旧课程，显示“无法读取课表数据”。
- 无学期或指定周无课程：属于正常空状态，不显示错误。
- 选择无效 UUID：保留当前有效选择，显示“所选学期不存在”。

失败时清理旧课程很重要，否则页面会同时显示新错误和上一次成功加载的数据。

## 6. 自动测试

测试覆盖：

- 活动/归档学期排序和存储失败。
- 所有模型角色及中文可读文本。
- 自动选择当前学期和计算当前教学周。
- 周日期范围、上一周、下一周和边界限制。
- 归档学期标签和主动切换学期。
- 空学期、当前日期不在学期内及无效选择。
- Repository 失败后清理旧状态并在恢复后重新加载。

MSVC Debug 构建成功，全工程 44/44 项 CTest 通过。

## 本课小结

当前依赖链保持为：

```text
QML（后续课程） -> ScheduleViewModel -> Application -> Domain
                                  Infrastructure -> Domain
```

下一课进入学期与课程管理 UI，并在应用组合根中组装真实 SQLite Repository、用例和
课表 ViewModel。
