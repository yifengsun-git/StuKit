# 第五十六课：待办与课表课程联动

## 本课目标

让新建作业能够选择课表中的真实课程，同时保持待办模块和课表模块的领域边界，避免
`TaskItem` 直接依赖 `schedule::Course`。

## 本课完成内容

### 1. `ICourseLookup` 只读端口

待办 Application 定义轻量 `CourseReference`：课程 ID、课程名、学期 ID 和学期名。
端口支持读取当前可选课程和按 ID 查找课程，并区分不存在与存储失败。

`SaveTask` 在组合根提供该端口时，会根据课程 ID 重新查询真实课程名并写入快照，不
信任界面提交的课程名称。这可以防止 ID 与名称不一致，也让名称快照有可靠来源。

### 2. `ScheduleCourseLookup` 适配器

适配器位于待办 Infrastructure，读取现有 `IAcademicTermRepository` 和
`ICourseRepository`，把课表实体翻译为待办可理解的课程引用。它只返回未归档学期中的
课程，并按学期名、课程名稳定排序。

依赖关系是单向的：

```text
Tasks Application <- ScheduleCourseLookup -> Schedule Repository 接口
```

课表 Domain 不知道任务存在，任务 Domain 也不知道完整课程实体。将来如果课程来源改为
远程教务系统，只需再实现一个 `ICourseLookup`。

### 3. 历史可读性

任务保存课程 UUID 的同时保存当时的课程名称快照。课程后来改名、学期归档或课程删除，
历史任务仍能显示原名称。编辑任务重新选择课程时才刷新快照。

## 测试覆盖

- 保存任务时使用查询到的真实课程名覆盖界面文本。
- 不存在的课程返回明确错误，不写入任务。
- 无课程任务仍不需要课表模块。

## 下一课

第 57 课实现 TasksViewModel：把用例转换为 QML 可消费的列表、分组、筛选、编辑状态和
统计摘要，并在组合根中装配真实 SQLite Repository 与课程适配器。
