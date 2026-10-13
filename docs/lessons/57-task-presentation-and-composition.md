# 第五十七课：任务 Presentation 与应用组合根

## 本课目标

把任务用例转换成 QML 可直接消费的数据，并用真实 SQLite、课表课程查询和用户时区
完成应用装配。

## 完成内容

`TasksViewModel` 提供任务列表、可选课程、待完成/已完成/逾期数量和完成率；支持分类、
关键字、归档筛选，以及保存、完成、重新打开、归档、恢复和删除操作。界面输入的
`yyyy-MM-dd HH:mm` 按用户记账时区解释，进入领域和数据库前转换成 UTC。

`TaskModule` 在组合根中创建 `SqliteTaskRepository`、课表 Repository、
`ScheduleCourseLookup`、全部用例和 ViewModel。它复用 AccountingModule 已打开并完成
迁移的数据库连接，不创建第二个数据库文件。数据恢复后组合根也会重新发布新的
TasksViewModel 指针。

## 下一课

第 58 课建立响应式任务页面，并接入首页和主导航。
