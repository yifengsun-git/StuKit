# 第二十课：账户与分类首次设置

## 本课定位

第十九课已经让记账首页读取真实数据库，但全新用户没有账户和分类，暂时无法创建
第一笔账单。本课计划完成记账基础数据的首次设置流程：

```text
AccountingHomePage / Setup Dialog
  -> AccountingSetupViewModel
  -> CreateAccount / CreateCategory / GetAccountingSetup
  -> SQLite Repository
  -> accounts / categories
```

本课不自动写入演示数据，也不实现新增账单表单。用户明确创建的数据才进入正式
数据库。

## 1. 本课目标

- 查询真实账户和分类，并区分空数据与数据库失败。
- 建立账户、分类列表模型。
- 建立首次设置 ViewModel。
- 从 QML 创建资金账户。
- 从 QML 创建收入或支出分类。
- 正确解析期初余额文本，不使用浮点数。
- 创建成功后刷新列表和首页状态。
- 把新 ViewModel 加入 `AccountingModule` 并注入 QML。

## 2. 为什么先做基础数据

一笔 Transaction 必须引用：

- 一个存在且未归档的资金账户。
- 一个存在且未归档、类型匹配的分类。

如果直接制作账单表单，下拉框将没有可选项，错误状态也难以解释。先建立账户与
分类，下一课的账单表单才能沿用真实列表。

## 3. 改进查询失败语义

当前 `IAccountRepository::findAll()` 和 `ICategoryRepository::findAll()` 只返回
vector，无法区分“查询成功但为空”和“数据库失败”。本课计划将返回值调整为：

```cpp
std::optional<std::vector<Account>>
std::optional<std::vector<Category>>
```

约定：

- 有值且为空：首次使用，没有基础数据。
- 有值且非空：查询成功。
- `std::nullopt`：存储失败或数据库行不符合领域规则。

内存实现、SQLite 实现、现有测试和调用者必须同步修改，不能只改接口声明。

## 4. GetAccountingSetup 用例

新增 Application 用例统一读取账户和分类：

```cpp
struct AccountingSetup
{
    std::vector<Account> accounts;
    std::vector<Category> categories;
};
```

用例只协调两个 Repository，不处理 QML，也不把实体转换成文本。任一查询失败时
返回 `StorageFailure`。

## 5. 金额输入解析

创建账户需要期初余额。QML 输入的是字符串，不能使用：

```text
text.toDouble() * 100
```

计划新增整数解析器，支持：

```text
0       -> 0 分
12      -> 1200 分
12.3    -> 1230 分
12.34   -> 1234 分
-12.34  -> -1234 分
```

拒绝：

- 超过两位小数。
- 多个小数点。
- 空文本或只有正负号。
- 超出 qint64 分值范围。
- 科学计数法、NaN 和 Infinity。

解析器属于 Presentation 输入转换，不改变 Money 的领域表示。

## 6. 列表模型

计划新增：

```text
AccountListModel
CategoryListModel
```

账户角色：

```text
accountId, name, openingBalanceText, archived
```

分类角色：

```text
categoryId, name, isIncome, archived
```

第一版页面只展示未归档项目，但模型仍保留归档状态，避免以后扩展管理页面时重新
定义数据结构。

## 7. AccountingSetupViewModel

主要属性：

```text
accounts, categories
hasAccounts, hasIncomeCategories, hasExpenseCategories
hasError, errorMessage
```

主要操作：

```text
load()
createAccount(name, openingBalanceText)
createCategory(name, isIncome)
```

创建成功后重新查询两个列表。ViewModel 把 Application 错误转换为页面提示，但
名称长度、空名称等领域规则仍由实体和用例判断。

## 8. QML 首次设置界面

记账首页在缺少基础数据时显示引导卡片：

- 没有账户：显示“创建资金账户”。
- 没有支出分类：显示“创建支出分类”。
- 没有收入分类：收入分类可稍后创建，但页面给出入口。

账户对话框字段：

- 账户名称。
- 期初余额文本，默认 `0.00`。

分类对话框字段：

- 分类名称。
- 收入/支出选择。

对话框只收集输入和展示错误，不直接创建 UUID 或调用 Repository。

## 9. 组合根更新

`AccountingModule` 将增加：

- `CreateAccount`
- `CreateCategory`
- `GetAccountingSetup`
- `AccountingSetupViewModel`

QML 根对象增加第二个明确依赖，例如 `accountingSetup`，再传给记账页面。创建基础
数据成功后，后续账单表单可以直接复用相同 ViewModel 的列表模型。

## 10. 测试计划

### Application

- 空数据库返回成功的空列表。
- 返回已有账户和收入、支出分类。
- 任一 Repository 查询失败返回 `StorageFailure`。

### Presentation

- 精确解析正数、负数和两位小数金额。
- 拒绝格式错误和溢出金额。
- 加载并更新账户、分类模型。
- 创建账户成功后刷新列表。
- 创建收入和支出分类后更新对应状态。
- 无效输入显示稳定提示。
- 存储失败可重试。

### 真实 SQLite

- 通过组合根和 ViewModel 创建账户、分类。
- 重启模块后数据仍然存在。
- 不向空数据库插入未授权默认记录。

预计新增一个测试目标：

```text
18/18 tests passed
```

## 11. 计划文件变化

可能新增：

```text
src/features/accounting/application/GetAccountingSetup.h/.cpp
src/features/accounting/presentation/AccountingSetupViewModel.h/.cpp
src/features/accounting/presentation/AccountListModel.h/.cpp
src/features/accounting/presentation/CategoryListModel.h/.cpp
src/features/accounting/presentation/MoneyInputParser.h/.cpp
tests/unit/accounting/AccountingSetupPresentationTest.cpp
```

需要修改：

```text
IAccountRepository.h
ICategoryRepository.h
内存与 SQLite Repository 实现
AccountingModule.h/.cpp
main.cpp
Main.qml
AccountingHomePage.qml
相关 CMake 和测试文件
```

## 12. 验收清单

- [ ] 空列表和存储失败具有不同语义。
- [ ] 金额文本解析全程不使用浮点数。
- [ ] 用户可以创建资金账户。
- [ ] 用户可以创建收入与支出分类。
- [ ] 创建成功后模型立即刷新。
- [ ] 数据保存在真实 SQLite，重启后仍存在。
- [ ] 不自动插入演示账户或分类。
- [ ] QML 不直接调用 Repository。
- [ ] 全量测试通过。
- [ ] 应用可启动并显示首次设置引导。
- [ ] 源码目录没有数据库或设置临时文件。
- [ ] 文档与代码导读同步更新。

## 13. 后续方向

完成第二十课后，用户将能从全新数据库建立真实账户和分类。第二十一课再制作新增
账单表单，复用这些列表完成第一笔真实收入或支出记录，并刷新月度首页。
