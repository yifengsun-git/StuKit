# ADR 0001：逐步拆分 CMake 构建目标

## 状态

已接受，2026-10-06。

## 背景

StuKit 最终会包含应用入口、QML 界面、记账领域、课表领域、数据访问和自动
测试。不同职责最终应成为边界明确的构建目标。

当前项目只有一个应用入口和一个 QML 模块。如果现在就创建多个静态库、QML
插件和子目录 CMakeLists.txt，会引入链接、插件注册和依赖传播知识，但还不能
带来独立测试或复用收益。

## 决定

应用外壳阶段暂时保留一个可执行目标和一个 QML 模块。源文件仍然按照 app、
shell、shared 和 features 目录划分职责。

第一次拆分发生在记账 Domain 和 Application 层开始开发时，预期形成：

```text
StuKit                  应用可执行目标
StuKitAccountingDomain  记账领域库
StuKitAccountingApp     记账应用用例库
StuKitTests             自动测试目标
```

Infrastructure 是否独立成库，由数据库实现和集成测试的实际需要决定。

## 结果

- 初学阶段的构建过程保持简单。
- 文件边界先建立，构建边界随后建立。
- 当模块可以独立测试时，再通过 CMake 强制依赖方向。
- 不因为追求目录数量而提前引入静态 QML 插件等复杂机制。
