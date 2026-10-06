# StuKit 架构说明

## 1. 架构选择

项目采用模块化单体，在每个功能模块内部使用 Clean Architecture，并在界面层
使用 MVVM。

这意味着程序仍是一个容易构建和部署的应用，但记账、课表等模块具有明确
边界。项目不采用微服务，也不在本地额外启动 Web 后端。

## 2. 前后端定义

在本项目中：

- 前端是 QML/Qt Quick，负责页面、控件、导航、动画和用户输入。
- 后端是 C++，负责业务规则、应用用例、数据访问和平台能力。
- ViewModel 是二者之间的桥梁。

一次新增账单操作的调用方向如下：

```text
QML 页面
  -> TransactionEditorViewModel
  -> CreateTransaction 用例
  -> ITransactionRepository 接口
  -> SqliteTransactionRepository
  -> SQLite
```

数据保存结果沿相反方向返回，ViewModel 再更新页面状态。

## 3. 四个主要层次

### Domain

包含实体、值对象、业务规则和 Repository 接口。它不能依赖 QML、Qt SQL
或具体操作系统。

初期允许使用少量 Qt Core 类型，例如 QString、QDate 和 QUuid，但业务规则
本身必须可以脱离界面测试。

### Application

包含用户用例，例如新增账单、查询月度流水、创建课程和检查课程冲突。
它负责安排业务流程，不处理页面样式或 SQL 语句。

### Infrastructure

包含 SQLite Repository、数据库迁移、设置、备份、文件和平台适配。这一层
实现 Domain 定义的接口。

### Presentation

包含 ViewModel 和提供给 QML 的列表模型。它管理加载、成功、空数据、验证
失败和系统错误等页面状态。

QML 文件属于表现层，但与 C++ 表现层分开放置，便于明确语言和职责边界。

## 4. 依赖规则

允许的主要依赖方向：

```text
QML -> Presentation -> Application -> Domain
Infrastructure --------------------------> Domain
```

禁止：

- QML 直接执行 SQL。
- Domain 引用 ViewModel 或 QML 页面。
- Application 创建具体 SQLite Repository。
- Repository 返回页面控件或依赖页面生命周期。
- 在 QML 中实现统计、金额计算、冲突检测等业务规则。

具体实现由 app/CompositionRoot 在应用启动时创建并连接。

## 5. 模块规划

```text
src/
  app/                  程序入口和对象组装
  shell/                应用外壳与主导航
  shared/               真正跨模块的公共能力
  features/
    accounting/         记账模块
    schedule/           课表模块
```

每个功能模块可以包含：

```text
domain/
application/
infrastructure/
presentation/
qml/
```

不要因为两个类暂时相似就立即放进 shared。只有已经被多个模块稳定使用的
概念才进入 shared，避免形成难以维护的公共杂物目录。

## 6. 数据设计原则

- 金额以整数最小货币单位保存，不使用浮点数。
- 业务记录使用 UUID，方便未来同步。
- 记录创建时间和修改时间。
- 数据库包含 schema_migrations 表。
- 数据库结构只能通过顺序迁移升级。
- 使用事务保证多条相关写入要么全部成功，要么全部失败。
- 数据库文件位置通过 QStandardPaths 获取，不写死 Windows 路径。

## 7. Android 兼容边界

- 主界面使用 Qt Quick，不使用 Qt Widgets 作为核心 UI。
- 核心流程不能依赖鼠标悬停、右键或物理键盘。
- 平台专属实现放在 infrastructure/platform 下。
- 文件选择、通知、分享等能力先定义抽象接口。
- 页面同时考虑宽屏和窄屏布局。
- 数据库和耗时文件操作不得长期阻塞 UI 线程。
- 记账 MVP 完成后就进行首次 Android 编译验证。

## 8. 测试策略

- Domain：大量快速单元测试。
- Application：用内存或模拟 Repository 测试用例。
- Infrastructure：使用临时 SQLite 数据库做集成测试。
- Presentation：测试 ViewModel 状态变化。
- QML：关键流程做少量 UI 测试，其他界面以人工检查为主。
