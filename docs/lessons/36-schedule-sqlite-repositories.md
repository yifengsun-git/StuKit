# 第三十六课：课表 SQLite Repository

## 本课目标

第 36 课为三个课表 Repository 接口提供正式 SQLite 实现。学期、课程和上课安排现在
可以写入版本 4 数据库，并在 Repository 重建后恢复。

## 1. 新增 Infrastructure 目标

```text
src/features/schedule/infrastructure/
  CMakeLists.txt
  repositories/
    SqliteAcademicTermRepository.h/.cpp
    SqliteCourseRepository.h/.cpp
    SqliteCourseMeetingRepository.h/.cpp
```

`StuKitScheduleInfrastructure` 只链接 `Qt6::Sql` 和 `StuKitScheduleDomain`。三个实现的
构造函数都只接收 `QSqlDatabase`，没有依赖记账 Domain、Application、QML 或
ViewModel。

集成测试使用已有迁移器创建临时版本 4 数据库，因此测试目标会同时链接数据库管理
代码；这不改变正式课表 Infrastructure 的依赖方向。

## 2. `SqliteAcademicTermRepository`

- 保存 UUID、名称、ISO 开始日期、总周数和归档状态。
- `save()` 使用 UPSERT，同 UUID 保存会更新原学期。
- 按 ID 查询缺失时返回 `nullopt`，但 `lastError()` 保持为空。
- 全部查询按“使用中优先、开始日期从新到旧、名称、UUID”稳定排序。
- 数据库行会重新经过 `AcademicTerm::create()`。

## 3. `SqliteCourseRepository`

- 保存所属学期、课程名、教师、教室和规范化颜色。
- 支持按 ID、按学期和查询全部。
- 按学期结果使用课程名和 UUID 稳定排序。
- SQLite 外键拒绝不存在的学期。
- 同 UUID UPSERT 会更新全部课程字段。

Domain 允许教师和教室为空。Qt 中默认构造的 `QString{}` 是 null 字符串，直接绑定会
写成 SQL `NULL`，与数据库的 `NOT NULL` 约束冲突。因此 Repository 会将 null
QString 明确转换为普通空文本 `''`。这是“业务可选字段”和“数据库非空文本列”之间
必须由映射层处理的差异。

## 4. `SqliteCourseMeetingRepository`

- 保存课程 UUID、星期、节次、周次和单双周。
- 支持按 ID、按课程和查询全部。
- 星期明确映射为 1～7。
- 每周、单周、双周明确映射为 0、1、2。
- 不依赖 C++ 枚举的隐式底层数值，避免以后调整枚举顺序破坏旧数据。
- 按课程结果依次按星期、起始节次、起始周和 UUID 等字段稳定排序。
- SQLite 外键拒绝不存在的课程。

## 5. 双重数据校验

数据库 CHECK 和外键负责第一道防线，Repository 读取时再次调用 Domain 工厂。即使
数据库文件被外部工具绕过约束修改，非法日期、空课程名、非法星期或单双周也不会被
交给 Application。

单条查询遇到损坏行返回 `nullopt` 并设置 `lastError()`；列表查询遇到任何损坏行会让
整次查询失败，不返回不完整列表。

## 6. 集成测试

新增三个独立测试：

- `SqliteAcademicTermRepositoryTest`
- `SqliteCourseRepositoryTest`
- `SqliteCourseMeetingRepositoryTest`

测试使用临时数据库验证：

- 所有字段保存与还原。
- 缺失 ID 与成功空筛选结果。
- 同 UUID 更新而不产生重复行。
- 按学期、按课程筛选和稳定排序。
- 外键拒绝未知学期或课程。
- Repository 重建后数据仍存在。
- 绕过 CHECK 写入损坏行后，Domain 重建会拒绝该行。
- 数据库关闭时返回失败并提供错误详情。

全工程测试数由 35 增加到 38。

## 7. 本课没有实现

- 课表 Application 创建、修改和查询流程。
- 将 Repository 连接到应用组合根。
- ViewModel 和 QML 页面。

下一课进入 Application 层，完成基础命令、冲突校验和指定教学周查询。
