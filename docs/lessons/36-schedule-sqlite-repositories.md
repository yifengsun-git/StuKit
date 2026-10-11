# 第三十六课：课表 SQLite Repository

## 本课目标

为第 34 课的三个 Repository 接口提供正式 SQLite 实现，让学期、课程和上课安排可以
写入第 35 课创建的表，并在应用重启后恢复。

## 1. 计划新增的实现

```text
src/features/schedule/infrastructure/
  CMakeLists.txt
  repositories/
    SqliteAcademicTermRepository.h/.cpp
    SqliteCourseRepository.h/.cpp
    SqliteCourseMeetingRepository.h/.cpp
```

每个 Repository 构造时接收 `QSqlDatabase` 连接，只链接 Qt SQL 和课表 Domain，不
引用记账 Domain、记账 Application、QML 或 ViewModel。

## 2. 字段映射

- UUID 使用不带花括号的稳定文本。
- `QDate` 使用 `Qt::ISODate`。
- `bool` 映射为 0/1。
- `Weekday` 明确映射为 1～7。
- `WeekPattern` 明确映射为 0～2，不依赖编译器隐式枚举布局。
- 写入前仍通过 Domain 实体保证规则，读取后重新调用实体工厂验证数据库行。

读取到无法还原成合法实体的行时，整次查询失败，不能把损坏数据悄悄交给界面。

## 3. 保存与查询

- `save()` 使用参数绑定和 UPSERT，同 UUID 更新已有数据。
- 学期支持按 ID和查询全部。
- 课程支持按 ID、按学期和查询全部。
- 上课安排支持按 ID、按课程和查询全部。
- 所有列表使用明确 `ORDER BY`，不依赖 SQLite 未定义的自然顺序。
- SQL 失败返回接口约定的失败结果，空列表仍表示成功。

## 4. 集成测试

三个 Repository 使用临时版本 4 数据库，验证：

- 所有字段保存后能无损还原。
- 同 UUID 保存执行更新而不是重复插入。
- 按学期、按课程筛选和稳定排序。
- 缺失 UUID 与成功空列表。
- 外键拒绝不存在的学期和课程。
- 非法数据库行不会生成半合法实体。
- 销毁并重建 Repository 后数据仍存在。

测试只使用临时数据库，不接触用户正式数据。

完成后，第 37 课开始课表 Application CRUD、当前学期和指定教学周查询。
