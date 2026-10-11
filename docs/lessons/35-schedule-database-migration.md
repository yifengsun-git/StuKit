# 第三十五课：课表 SQLite 数据库迁移

## 本课目标

在现有记账数据库版本 3 的基础上增加版本 4，为课表建立正式数据表、约束、外键和
索引。只修改数据库结构和迁移测试，暂不编写 SQLite Repository。

## 1. 为什么继续使用同一个数据库

StuKit 是模块化单体应用。记账和课表业务代码彼此独立，但都属于同一个用户、同一个
应用数据目录，因此可以共用数据库连接和 `schema_migrations`。课表表使用独立名称和
外键，不会直接依赖记账表。

## 2. 计划新增的表

### `academic_terms`

- `id`：UUID 文本主键。
- `name`：学期名称。
- `start_date`：ISO 日期文本。
- `total_weeks`：1～30。
- `archived`：0 或 1。

### `courses`

- `id`：UUID 文本主键。
- `term_id`：关联 `academic_terms`。
- `name`、`teacher`、`location`。
- `color`：规范化 `#RRGGBB` 文本。

### `course_meetings`

- `id`：UUID 文本主键。
- `course_id`：关联 `courses`。
- `weekday`：1～7。
- `start_period`、`end_period`：1～20，且起始不晚于结束。
- `start_week`、`end_week`：1～30，且起始不晚于结束。
- `week_pattern`：每周、单周、双周的稳定整数编码。

节次的具体起止时间尚未进入 Domain，因此本次不提前创建无代码使用的
`period_definitions` 表；以后实现可自定义作息时间时再通过新迁移加入。

## 3. 外键和删除策略

```text
academic_terms 1 ---- N courses 1 ---- N course_meetings
```

- 更新父 UUID 时使用 `ON UPDATE CASCADE`。
- 删除仍被课程引用的学期时使用 `ON DELETE RESTRICT`。
- 删除仍有安排的课程时使用 `ON DELETE RESTRICT`。
- 日常操作优先归档学期；真正的级联删除必须以后设计成明确用例。

## 4. 索引

至少建立：

- `courses(term_id)`：按学期读取课程。
- `course_meetings(course_id)`：按课程读取安排。
- `course_meetings(weekday)`：辅助按星期构建周课表。

## 5. 迁移测试

- 新数据库直接迁移到版本 4。
- 已有版本 1、2、3 数据库都能升级到版本 4。
- 旧账户、分类和账单数据保持不变。
- 重复调用迁移不会重复建表或记录版本。
- 表字段、检查约束、外键和索引存在。
- 非法星期、节次、周次、单双周和无效外键写入失败。
- 迁移失败时事务回滚，不留下半套课表表结构。

完成后，第 36 课实现三个 SQLite Repository 和实体映射。
