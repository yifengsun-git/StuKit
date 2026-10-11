# 第二十九课：课表需求与核心时间模型

## 本课定位

记账 MVP 已完成日常操作、备份恢复和 CSV 导出。正式 Windows 部署推迟到课表 MVP
完成之后，避免功能和依赖仍在快速变化时反复调整发布目录。本课开始第二个正式模块：
课表。

和记账模块一样，课表先从不依赖界面的 Domain 开始。第一步不画课表网格，也不写
SQLite，而是把“星期几、第几节、第几周、单双周”这些最容易出错的规则定义清楚，
并用自动测试固定下来。

## 1. 课表 MVP 的用户目标

第一版课表要支持：

- 创建和切换学期。
- 创建课程，填写课程名、教师和地点。
- 为课程设置星期、起止节次、起止周和单双周。
- 查看指定教学周的周课表。
- 修改或删除课程安排。
- 在保存前发现时间冲突。
- 应用重启后从 SQLite 恢复数据。

第一版暂不支持教务系统导入、提醒、系统日历同步、桌面组件和跨设备云同步。

## 2. 先统一术语

课表中需要区分三个概念：

```text
Course（课程）
  高等数学、大学英语等课程资料

CourseMeeting（上课安排）
  某门课程在星期几、第几节、哪些教学周上课

AcademicTerm（学期）
  学期名称、开学日期和总教学周数
```

一门课程可以有多个上课安排。例如“高等数学”可能周一 1～2 节在 A101，周四
3～4 节在 B202。因此不能把星期和节次直接写死在 Course 中。

## 3. 本课实现范围

本课只实现三个核心时间类型：

### `Weekday`

使用明确枚举表示星期一到星期日，不在 Domain 中保存“周一”“Monday”等显示文本。
显示文本以后由 Presentation 转换。

```cpp
enum class Weekday
{
    Monday = 1,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};
```

### `ClassPeriodRange`

表示连续节次范围，例如第 1～2 节：

- 节次从 1 开始。
- 第一版最多允许 20 节，为不同学校留出空间。
- 开始节次不能晚于结束节次。
- 提供 `contains()` 和 `overlaps()`，为后续冲突检测准备。

### `TeachingWeekRange`

表示教学周范围和重复规则：

```text
第 1～16 周，每周
第 1～15 周，单周
第 2～16 周，双周
```

- 教学周从 1 开始，第一版最多允许 60 周。
- `WeekPattern` 分为 EveryWeek、OddWeeks、EvenWeeks。
- 单双周按学期的绝对周数判断，不按范围起点重新计数。
- 提供 `occursInWeek()` 和 `overlaps()`。

## 4. 为什么使用值对象

如果页面和数据库各自保存几个裸整数，就很容易产生“第 0 周”“结束节次早于开始
节次”等无效状态。值对象通过工厂函数创建，只有合法组合才能成为对象：

```text
QML 输入
  -> Application 请求
  -> Domain 值对象验证
  -> 合法对象
```

这些类型不依赖 QML、SQLite 或 Windows，因此未来 Android 可以直接复用。

## 5. 计划中的文件结构

```text
src/features/schedule/
  domain/
    CMakeLists.txt
    Weekday.h
    ClassPeriodRange.h
    ClassPeriodRange.cpp
    TeachingWeekRange.h
    TeachingWeekRange.cpp

tests/unit/schedule/
  CMakeLists.txt
  ClassPeriodRangeTest.cpp
  TeachingWeekRangeTest.cpp
```

顶层 CMake 会新增 `StuKitScheduleDomain` 静态库和课表单元测试目录，但不会让课表
Domain 依赖现有记账模块。

## 6. 规则细节

### 节次重叠

```text
1～2 与 2～3：重叠（都包含第 2 节）
1～2 与 3～4：不重叠
```

### 教学周重叠

教学周范围相交还不够，重复规则也必须在某个相同周成立：

```text
1～8 单周 与 2～8 双周：不冲突
1～8 单周 与 5～10 每周：在第 5、7 周冲突
2～8 双周 与 6～10 双周：在第 6、8 周冲突
```

后续 `CourseMeeting` 的冲突检测会同时检查：同一学期、同一星期、节次重叠、教学周
重叠。地点是否相同不会影响个人课表的时间冲突判断。

## 7. 自动测试计划

`ClassPeriodRangeTest`：

- 接受 1～1、1～2 和 19～20。
- 拒绝第 0 节、超过 20 节和反向范围。
- 验证边界包含与节次重叠。

`TeachingWeekRangeTest`：

- 接受合法的每周、单周和双周范围。
- 拒绝第 0 周、超过 60 周和反向范围。
- 验证绝对周数的单双周判断。
- 验证范围相交但单双周不相交的情况。
- 验证 EveryWeek 与单双周的交集。

## 8. 本课不做什么

- 不创建学期、课程或上课安排实体。
- 不修改 SQLite 数据库版本。
- 不创建课表编辑页面。
- 不实现课程冲突提示。

这些内容分别在后续小课中完成，避免一次同时处理领域规则、数据库和 UI。

## 9. 验收清单

- [ ] `Weekday` 只表达稳定业务值，不包含显示文案。
- [ ] 节次范围不能进入无效状态。
- [ ] 教学周范围和单双周规则明确。
- [ ] 两种范围都能判断包含和重叠。
- [ ] 课表 Domain 不依赖记账、QML 或 Qt SQL。
- [ ] 新增自动测试全部通过。
- [ ] 原有 27 个记账测试继续通过。
- [ ] C++ 代码导读同步更新。

## 10. 后续课程路线

```text
第 29 课  星期、节次和教学周值对象
第 30 课  AcademicTerm 学期实体与业务规则
第 31 课  Course 与 CourseMeeting 实体
第 32 课  课程冲突检测用例
第 33 课  课表 Repository 接口与内存测试替身
第 34 课  SQLite 迁移与课表 Repository
第 35 课  课表 ViewModel 和指定周查询
第 36 课  周课表界面与课程编辑流程
第 37 课  课表 MVP 收尾与数据可靠性
第 38 课  Windows Release 构建和文件夹部署
```

第 38 课会详细覆盖 Release 配置、CMake 安装规则、`windeployqt`、QML 模块、
`qwindows.dll`、`qsqlite.dll`、MSVC 运行库、隔离启动验证和发布目录检查；届时应用
功能边界稳定，部署结果才具有长期价值。
