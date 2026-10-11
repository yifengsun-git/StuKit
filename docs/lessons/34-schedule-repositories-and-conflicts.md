# 第三十四课：课表 Repository 与冲突检测

## 本课目标

第 34 课为三个课表实体建立存取契约和内存测试实现，并完成不依赖 SQLite、QML 的
课程冲突检测。完整模块路线见[课表模块开发计划](../SCHEDULE_PLAN.md)。

## 1. 为什么需要 Repository 接口

Application 需要读取学期、课程和安排，但不应该知道数据保存在 SQLite、内存还是
以后其他设备的数据源中。三个接口放在 Domain：

- `IAcademicTermRepository`：保存、按 UUID 查询、查询全部学期。
- `ICourseRepository`：保存、按 UUID 查询、按学期查询、查询全部课程。
- `ICourseMeetingRepository`：保存、按 UUID 查询、按课程查询、查询全部安排。

`save()` 使用相同 UUID 更新已有对象；筛选查询成功但没有数据时返回空 `vector`；
全部查询使用 `optional<vector<...>>`，以区分“查询成功但为空”和“存储失败”。接口
没有表名、SQL 语句或数据库连接。

## 2. 内存 Repository

`tests/unit/schedule/support/` 中的三个内存实现只服务自动测试：

- 使用 `std::vector` 保存实体。
- 支持新增、同 UUID 更新、按 ID 和关联 ID 查询。
- 保留插入顺序。
- 可以模拟保存失败或查询失败。
- 记录 `findAll()` 调用次数，用于防止用例退化为重复查询。

程序退出后内存数据会消失，因此它们不能代替第 35～36 课的 SQLite 实现。

## 3. `DetectScheduleConflicts` 用例

用例接收一条尚未保存或正在编辑的安排数据，先通过 `CourseMeeting::create()` 验证
输入，然后分别读取一次学期、课程和安排快照。

两条安排同时满足以下条件才冲突：

1. 关联课程属于同一学期。
2. 星期相同。
3. 节次闭区间重叠。
4. 周次范围有交集。
5. 交集内至少有一周同时满足双方的每周、单周或双周模式。

例如：

- 周一第 1～2 节与周一第 2～3 节在第 2 节冲突。
- 周一第 1～2 节与周一第 3～4 节不冲突。
- 相同星期和节次的纯单周、纯双周安排互不冲突。
- “每周”与“单周”会在双方范围中的第一个单周冲突。
- 不同学期可以在相同星期和节次上课。

结果中的每项冲突同时包含课程、上课安排和第一个冲突周次，方便以后界面直接显示
可读课程名。结果按第一个冲突周、起始节次、课程名和 UUID 稳定排序。

## 4. 编辑与数据完整性

编辑已有安排时，请求沿用原 UUID。检测器会排除相同 UUID 的已存安排，避免把对象
自己报告为冲突。

若 Repository 中存在引用未知课程的安排，说明持久化数据已经破坏了关联完整性。
用例返回 `StorageFailure`，不会静默忽略。第 35 课还会使用 SQLite 外键从数据库层
阻止这种情况。

## 5. 错误结果

`DetectScheduleConflictsError` 明确区分：

- `InvalidInput`：UUID、星期、节次、周次或单双周输入无效。
- `CourseNotFound`：候选安排关联的课程不存在。
- `TermNotFound`：课程关联的学期不存在。
- `MeetingExceedsTerm`：安排结束周超过学期总周数。
- `StorageFailure`：任一 Repository 查询失败或读取到损坏的关联。

成功且没有冲突返回空冲突列表，不属于错误。

## 6. 自动测试

本课新增：

- `ScheduleRepositoryTest.cpp`：新增、更新、按关联筛选、空结果和失败模拟。
- `DetectScheduleConflictsTest.cpp`：星期、节次、周次、单双周、跨学期、编辑排除、
  稳定排序、错误分类、损坏关联及固定查询次数。

本课完成后，全工程共有 35 个 CTest 测试。

## 7. 本课没有实现

- SQLite 表和正式 Repository。
- 创建、修改、归档课程的完整用例。
- ViewModel 和课表界面。

下一课为共享 SQLite 数据库增加课表迁移、外键、约束和索引。
