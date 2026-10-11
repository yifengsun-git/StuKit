# 第三十三课：课表领域模型

## 本课目标

记账与分析 MVP 已经完成。第 33 课开始课表模块，先建立不依赖 QML、SQLite 或平台的
领域模型和自动测试，为 Windows 与 Android 共用同一套课程规则。

## 1. 领域概念

计划建立：

- `AcademicTerm`：学期名称、开始日期、总周数。
- `Course`：课程名称、教师、教室和显示颜色。
- `CourseMeeting`：某门课程在星期几、第几节、哪些周上课。
- `WeekPattern`：每周、单周、双周。
- 课程、学期和上课安排各自使用稳定 UUID。

## 2. 关键业务规则

- 学期名称和课程名称不能为空，并限制最大长度。
- 学期开始日期必须有效，总周数在合理范围内。
- 星期限制为星期一到星期日。
- 起始节次不能晚于结束节次。
- 起始周不能晚于结束周，且不能超过学期周数。
- 单双周规则必须与周次范围共同判断。
- 颜色使用稳定、可持久化的文本编码，不把 QML `color` 类型带入 Domain。

## 3. 冲突的定义

本课先让模型能够描述冲突所需的数据。两个安排只有同时满足以下条件才可能冲突：

1. 属于同一学期。
2. 星期相同。
3. 节次范围重叠。
4. 至少有一个实际周次同时上课。

完整冲突检测会放在后续 Application 用例中，避免单个实体查询其他课程。

## 4. 计划目录

```text
src/features/schedule/domain/
  CMakeLists.txt
  AcademicTerm.h/.cpp
  Course.h/.cpp
  CourseMeeting.h/.cpp
  WeekPattern.h

tests/unit/schedule/
  CMakeLists.txt
  AcademicTermTest.cpp
  CourseTest.cpp
  CourseMeetingTest.cpp
```

## 5. 本课不做什么

- 不创建数据库表。
- 不绘制课表界面。
- 不实现教务系统导入。
- 不实现提醒或日历同步。
- 不把时间安排简化成界面坐标。

完成本课后，下一课将设计课表 Repository 接口和课程冲突检测用例。
