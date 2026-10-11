# 第三十三课：课表领域模型

## 本课目标

记账与分析 MVP 已经完成。第 33 课正式启动课表模块，先建立不依赖 QML、SQLite
或操作系统的领域模型，并用自动测试固定核心规则。Windows 与以后 Android 版本会
共用这一层代码。

完整模块范围、页面规划和第 33～42 课安排见
[课表模块开发计划](../SCHEDULE_PLAN.md)。

## 1. 为什么先写 Domain

课表页面最终只负责展示和收集输入，不能决定“单双周是否上课”“节次范围是否合法”
等规则。先完成 Domain 有三个好处：

- 业务规则可以脱离界面单独测试。
- SQLite 只是保存实体，不会成为规则来源。
- Android 改变页面布局时，无须重写课程规则。

当前依赖方向为：

```text
未来的 QML -> Presentation -> Application -> Schedule Domain
未来的 SQLite Infrastructure ----------------> Schedule Domain
```

## 2. 本课完成的类型

### `AcademicTerm`

表示一个学期，保存稳定 UUID、名称、开始日期、总周数和归档状态。

创建时保证：

- UUID 非空。
- 名称清除首尾空白后仍非空，最长 40 个字符。
- 开始日期有效。
- 总周数处于 1～30 周。

归档不会删除历史课程，它表示该学期不再作为日常使用的当前学期候选。

### `Course`

表示某学期中的一门课，保存课程 UUID、学期 UUID、名称、教师、教室和颜色。

- 课程名必填，教师和教室可为空。
- 文本会清除首尾空白。
- 颜色必须是 `#RRGGBB`，并统一保存为大写，例如 `#3b82f6` 变为 `#3B82F6`。
- Domain 使用字符串颜色，不依赖 QML `color` 或桌面平台类型。

### `Weekday` 与 `WeekPattern`

`Weekday` 使用星期一到星期日七个稳定枚举值。`WeekPattern` 表示：

- `EveryWeek`：每周。
- `OddWeeks`：单周，即第 1、3、5……周。
- `EvenWeeks`：双周，即第 2、4、6……周。

`matchesWeek()` 只判断某个正周次是否匹配模式，不查询数据库或页面状态。

### `CourseMeeting`

表示一门课程的一组重复上课安排，保存课程 UUID、星期、节次范围、周次范围和
单双周模式。一门课程以后可以拥有多条安排。

创建时保证：

- 安排 UUID 和课程 UUID 非空。
- 星期处于星期一到星期日。
- 节次处于 1～20，且起始节次不晚于结束节次。
- 周次处于 1～30，且起始周不晚于结束周。
- 周次范围内至少有一周匹配单双周模式。

`occursInWeek(week)` 会同时考虑起止周和单双周，可直接回答某条安排在指定教学周
是否实际上课。

## 3. 为什么没有在实体里判断学期总周数

`CourseMeeting` 本身只有 `courseId`，并不知道课程属于哪个学期，也不知道该学期
是 16 周还是 20 周。让实体自己查询其他对象会造成隐藏依赖。

因此本课只设置系统级上限 30 周。下一阶段的 Application 用例同时拿到安排、课程和
学期后，再验证 `endWeek <= AcademicTerm::totalWeeks()`。同理，跨课程冲突也属于
Application，不属于单个实体。

## 4. 文件结构

```text
src/features/schedule/domain/
  CMakeLists.txt
  AcademicTerm.h/.cpp
  Course.h/.cpp
  CourseMeeting.h/.cpp
  Weekday.h
  WeekPattern.h

tests/unit/schedule/
  CMakeLists.txt
  AcademicTermTest.cpp
  CourseTest.cpp
  CourseMeetingTest.cpp
```

`StuKitScheduleDomain` 是独立静态库，目前只公开链接 `Qt6::Core`。它没有链接记账、
SQL、Quick 或 Charts。

## 5. 自动测试覆盖

- 学期名称、日期、周数、UUID 和归档状态。
- 课程关联、可选教师/教室、文本清理和颜色规范化。
- 非法星期、节次和周次范围。
- 每周、单周、双周在范围内外的实际发生判断。
- 单周模式配合纯双周范围等“没有任何实际周次”的无效输入。

本课完成后全工程共有 33 个 CTest 测试，全部通过。

## 6. 本课刻意没有实现

- 数据库表和持久化。
- 课程冲突检测。
- 创建、修改和查询用例。
- 课表 ViewModel 与 QML 页面。
- 教务导入、提醒和日历同步。

这些内容按依赖顺序逐课加入。下一课建立 Repository 接口、内存测试实现和冲突检测
用例。
