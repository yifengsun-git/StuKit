# 第三十九课：学期与课程管理 UI

## 本课目标

把课表模块接入真实应用，使用户可以在 QML 中创建和维护学期、课程及重复上课安排。
本课优先完成管理流程，第 40 课再制作完整周课表视觉布局。

## 1. 课表组合根

计划建立独立的 `ScheduleModule`：

- 复用应用现有 SQLite 连接和版本 4 数据库结构。
- 创建三个课表 SQLite Repository。
- 创建保存、查询和冲突检测相关 Application 用例。
- 创建 `ScheduleViewModel` 和管理表单 ViewModel。
- 按正确析构顺序管理 Repository、用例与 Presentation 生命周期。

记账模块与课表模块共用数据库连接，但不互相持有业务对象。

## 2. 学期管理

界面需要支持：

- 查看活动和已归档学期。
- 新建学期：名称、开始日期、总周数。
- 修改学期资料。
- 归档和恢复学期。
- 缩短周数时展示已有安排越界的明确错误。

## 3. 课程与安排管理

- 创建和编辑课程名称、教师、教室和颜色。
- 一门课程可以添加多条上课安排。
- 设置星期、起止节次、起止周及每周/单周/双周。
- 冲突时展示冲突课程、节次和首次冲突周，不写入数据。
- 已归档学期不能新增课程，但历史数据仍然可查看。

## 4. QML 结构

计划将管理界面拆成小组件，避免把所有表单放进 `ScheduleHomePage.qml`：

```text
ScheduleHomePage.qml
components/TermManager.qml
components/TermEditorDialog.qml
components/CourseManager.qml
components/CourseEditorDialog.qml
components/MeetingEditor.qml
```

所有按钮和表单兼顾鼠标与触摸操作，不依赖悬停、右键或双击。

## 5. 验证计划

- 使用真实临时 SQLite 验证组合根重启后数据恢复。
- Presentation 测试覆盖创建、编辑、归档和冲突错误映射。
- QML 静态检查无警告。
- 手工验证从空数据库创建学期、课程和安排的完整流程。

完成后，第 40 课实现桌面周网格和 Android 友好的按天布局。
