# 第三十七课：课表 Application 用例

## 本课成果

本课把 Domain、冲突检测和 Repository 契约组合成了四个可直接供 Presentation
调用的业务入口。界面以后不需要知道 SQLite 表结构，也不需要自己拼接跨实体规则。

新增用例：

- `SaveAcademicTerm`：创建或修改学期，并保护已有课程安排不被缩短到学期范围外。
- `SaveCourse`：创建或修改课程，校验所属学期存在且未归档。
- `SaveCourseMeeting`：创建或修改上课安排，保存前完成关联、周数和冲突检查。
- `GetWeeklySchedule`：查询指定学期、指定教学周真正发生的课程。

## 1. 为什么不直接让 QML 调 Repository

Repository 只负责存取数据，不知道一次用户操作需要哪些步骤。例如保存上课安排时，
必须依次确认：

1. 编辑的安排确实存在。
2. 所属课程存在。
3. 所属学期存在。
4. 安排没有超过学期总周数。
5. 与同一学期其他安排没有实际教学周冲突。
6. 所有检查通过后才保存。

这是一条完整业务流程，因此属于 Application。QML 只负责收集输入、调用用例并展示
结果；SQLite Repository 只负责可靠读写。

## 2. 保存学期与课程

`SaveAcademicTermRequest.id` 为空表示创建，非空表示更新。创建时用例生成新 UUID；
更新时保留原 UUID。若缩短总周数，用例会读取该学期的课程及安排，只要有一条安排的
结束周超过新总周数，就返回 `HasOutOfRangeMeetings`，不会写入部分数据。

`SaveCourse` 使用相同的创建/更新入口。课程必须属于存在且未归档的学期，已有课程也
不能借更新操作偷偷移动到另一学期。以后若确实需要“迁移课程”，应建立语义明确的
独立用例。

## 3. 保存上课安排与冲突结果

`SaveCourseMeeting` 复用第 34 课的 `DetectScheduleConflicts`。结果使用
`std::variant` 表达三种互斥状态：

- `CourseMeeting`：无冲突，已经保存成功。
- `CourseMeetingConflicts`：发现冲突，返回冲突课程和首次冲突周，不保存。
- `SaveCourseMeetingError`：输入、关联或存储发生错误。

冲突是可预期的业务结果，不等同于数据库故障。把它单独建模后，第 39～41 课的界面
可以给用户明确提示，而不需要解析错误字符串。

## 4. 指定教学周查询

`GetWeeklySchedule` 输入学期 UUID 和教学周数。它各读取一次学期、课程和安排快照，
使用 `CourseMeeting::occursInWeek()` 过滤每周、单周和双周规则，再返回平台无关的
`WeeklyScheduleItem`：

- 课程与安排 UUID。
- 课程名、教师、教室和颜色。
- 星期、起始节次和结束节次。

结果按星期、起始节次、结束节次、课程名和 UUID 稳定排序。空学期或某周没有课程是
成功的空列表，不是错误。Application 只返回普通 C++ 数据，`QAbstractListModel` 留给
下一课的 Presentation。

## 5. 自动测试

新增四个独立 Qt Test 目标，覆盖：

- 创建、更新和 UUID 保留。
- 学期归档、缩短总周数保护和已归档学期限制。
- 安排冲突时不保存，单双周错开时允许保存。
- 编辑安排时排除自身冲突。
- 指定周过滤、可读字段组装和稳定顺序。
- 无效输入、缺失关联、越界周数和全部 Repository 失败路径。
- 查询次数固定且可解释。

全工程 Debug 构建成功，42/42 项 CTest 通过。

## 本课小结

课表已经具备了不依赖 UI 和 SQLite 实现的完整核心操作。依赖方向仍然是：

```text
Presentation（下一课） -> Application -> Domain
Infrastructure --------------------------> Domain
```

下一课进入 `Presentation`：把 `WeeklySchedule` 转成 QML 可绑定的列表模型，并建立周
切换、学期选择和错误状态的 ViewModel。
