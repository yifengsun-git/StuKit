# 第三十八课：课表 Presentation 与周视图模型

## 本课目标

在不编写完整课表页面的前提下，建立 QML 与课表 Application 之间的状态桥梁。
本课继续先写 C++ 和自动测试，第 39、40 课再接入具体管理表单与周课表界面。

## 1. 周课表列表模型

计划建立 `WeeklyScheduleListModel`，将 `WeeklyScheduleItem` 暴露为稳定角色：

- `courseId`、`meetingId`。
- `courseName`、`teacher`、`location`、`color`。
- `weekday`、`weekdayText`。
- `startPeriod`、`endPeriod`、`periodText`。

模型只负责数据转换和通知 QML，不查询数据库，也不重新判断单双周。

## 2. 课表 ViewModel

计划建立 `ScheduleViewModel`，负责：

- 加载可用学期并选中当前学期。
- 根据学期开始日期计算当前教学周。
- 加载指定教学周的课表。
- 上一周、下一周和返回当前周。
- 暴露学期名、周次范围、空状态、加载状态和错误文本。
- 学期或课程数据修改后统一刷新。

当前学期选择需要持久化时，由应用组合根或用户设置负责；ViewModel 不直接读写
`QSettings`。

## 3. 错误映射

Application 的稳定错误枚举将在 Presentation 转换为用户可读文本。失败后要清理已经
过期的列表，避免页面同时显示旧课程和新错误。指定周没有课程仍然是正常空状态。

## 4. 响应式后手

列表模型不绑定桌面网格坐标。它提供星期和节次等语义数据，宽屏 QML 可以排成周网格，
Android 窄屏则可以切换为按天列表，两种布局复用同一个 ViewModel。

## 5. 自动测试计划

- 每个模型角色与可读文本。
- 初次加载、空课表和错误清理。
- 上一周、下一周及边界限制。
- 当前教学周的日期计算。
- 切换学期后周数和列表同步刷新。
- Repository 失败映射为稳定页面状态。
- Application 与 Presentation 的依赖方向不反转。

完成后，第 39 课进入学期与课程管理 UI。
