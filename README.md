# StuKit

StuKit 是一个面向大学生日常使用的个人工具包。项目首先开发 Windows
桌面版本，之后适配 Android。

当前规划的功能：

- 记账本：账户、分类、收支流水和月度统计。
- 课表：学期、课程、周次、节次和课程冲突检测。
- 后续模块：计时器、待办等，按实际需求逐步增加。

## 当前状态

项目处于工程骨架阶段，已经完成 Qt Quick 应用入口、基础导航、主题系统和通用
占位页组件，尚未开始实际业务代码开发。

当前目标是搭建 Qt 6、CMake、C++ 和 QML/Qt Quick 工程，并优先完成记账本
MVP。开发过程中保持界面与业务分离，为 Android 迁移预留空间。

## 技术方向

- 开发环境：Qt Creator
- UI：QML、Qt Quick Controls
- 业务逻辑：C++17 或 C++20
- 构建系统：CMake
- 本地数据库：SQLite、Qt SQL
- 架构：模块化单体、Clean Architecture、MVVM
- 测试：Qt Test

## 文档入口

- [项目计划](docs/PROJECT_PLAN.md)
- [架构说明](docs/ARCHITECTURE.md)
- [开发协作方式](docs/DEVELOPMENT_GUIDE.md)
- [第一课：准备开发环境](docs/lessons/01-environment-setup.md)
- [第二课：创建正式工程骨架](docs/lessons/02-project-bootstrap.md)
- [第三课：建立应用导航](docs/lessons/03-app-navigation.md)
- [第四课：建立主题与通用组件](docs/lessons/04-theme-and-shared-components.md)
- [第五课：Money 值对象与自动测试](docs/lessons/05-money-and-unit-tests.md)

## 开发原则

1. QML 只负责界面展示与交互。
2. C++ 负责业务规则、用例和数据访问。
3. 核心业务不依赖具体页面或 SQLite 实现。
4. 所有数据库变更都通过版本迁移完成。
5. 每次只实现一个可验证的小目标。
