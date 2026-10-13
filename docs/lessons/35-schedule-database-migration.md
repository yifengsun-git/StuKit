# 第三十五课：课表 SQLite 数据库迁移

## 本课目标

第 35 课把共享 SQLite 数据库从版本 3 升级到版本 4，为课表建立正式表结构、约束、
外键和索引，并同步升级备份验证。这里只负责数据库结构，暂不编写 SQLite Repository。

## 1. 为什么继续使用同一个数据库

StuKit 是模块化单体应用。记账和课表 Domain 彼此独立，但都属于同一个用户和应用
数据目录，因此共用一条 SQLite 连接与 `schema_migrations`。课表使用独立表和外键，
不会引用账户、分类或账单。

当前迁移器仍位于已有的 accounting infrastructure 目录，这是项目早期形成的物理
位置；新增的课表表不依赖记账业务类型。第 36 课的课表 Repository 将只接收
`QSqlDatabase`，不会反向依赖记账 Domain 或 Application。

## 2. 版本 4 新增表

### `academic_terms`

- UUID 文本主键。
- 非空且最长 40 字符的学期名称。
- 标准 ISO 日期格式的开始日期。
- 1～30 的总周数。
- 只能为 0 或 1 的归档状态。

### `courses`

- UUID 文本主键。
- 指向 `academic_terms` 的 `term_id`。
- 必填课程名、可为空的教师和教室。
- 规范化的大写 `#RRGGBB` 颜色。

### `course_meetings`

- UUID 文本主键。
- 指向 `courses` 的 `course_id`。
- 1～7 的星期。
- 1～20 且顺序正确的起止节次。
- 1～30 且顺序正确的起止周。
- 0、1、2 三个稳定单双周编码。

节次的具体起止时间还未进入 Domain，因此没有提前创建无人使用的
`period_definitions` 表；以后实现自定义作息时间时再通过新迁移加入。

## 3. 外键、删除策略和索引

```text
academic_terms 1 ---- N courses 1 ---- N course_meetings
```

- 父 UUID 更新使用 `ON UPDATE CASCADE`。
- 删除仍被引用的学期或课程使用 `ON DELETE RESTRICT`。
- 日常历史学期优先归档；真正删除必须以后通过明确用例处理。
- `courses(term_id)` 支持按学期查询。
- `course_meetings(course_id)` 支持按课程查询。
- `course_meetings(weekday)` 支持构建周课表。

## 4. 原子迁移

建三张表、建三个索引和写入版本 4 记录全部处于同一事务。如果任意语句失败，事务
回滚，不留下半套表结构。重复调用 `migrate()` 不会再次执行版本 4。

迁移器还新增了未来版本保护：完成本课时程序只支持到版本 4，若打开更高版本的数据库，
会明确失败，防止旧程序误读新结构。项目当前最高版本请以
`SqliteDatabase::CurrentSchemaVersion` 为准。

## 5. 备份兼容性

完成本课时，`SqliteBackupService` 的最高支持版本同步为 4：

- 版本 4 备份必须包含三张课表表。
- 版本 1～3 的旧备份仍可通过验证，恢复后由迁移器升级。
- 当时的版本 5 及以上返回 `UnsupportedVersion`；后续迁移会同步提高这一上限。

数据库当前版本只由 `SqliteDatabase::CurrentSchemaVersion` 定义，迁移器和备份服务不再
各自维护容易不一致的数字。

## 6. 自动测试

`SqliteDatabaseTest` 现在验证：

- 新数据库直接建立版本 4 的字段、索引和外键。
- 版本 1、2、3 均能升级到版本 4，并保留旧记账数据。
- 非法日期、颜色、星期、节次、周次、单双周和外键写入被拒绝。
- 被安排引用的课程不能直接删除。
- 重复迁移保持 4 条唯一版本记录。
- 迁移中途失败会完整回滚。
- 未来数据库版本会被拒绝。

备份和应用组合根测试也同步到版本 4。全量测试通过后进入第 36 课，实现三个 SQLite
Repository 和实体映射。
