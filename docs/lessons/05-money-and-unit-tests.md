# 第五课：Money 值对象与自动测试

## 本课目标

本课开始编写真正的记账后端代码。我们先实现一个很小但非常重要的领域概念：
金额 `Money`。

同时建立第一个独立 C++ 领域库和 Qt Test 测试目标，让业务规则能够在不启动
QML 界面的情况下验证。

完成后，你应该理解：

- 为什么金额不能直接使用 `double`。
- 什么是值对象。
- 头文件和实现文件如何分工。
- 静态库目标如何建立模块边界。
- 单元测试如何验证业务代码。
- CTest、Qt Test 和普通应用目标分别做什么。

本课不会把 Money 暴露给 QML，也不会连接数据库。

## 1. 为什么先实现 Money

如果使用浮点数表示金额，某些十进制数无法被二进制浮点精确表示。例如多次
计算后可能出现本应为 `0.30`、实际却略大或略小的结果。

本项目以最小货币单位保存金额：

```text
1 分      -> 1
1.00 元   -> 100
123.45 元 -> 12345
```

Money 只保存整数“分”。将来界面负责把它格式化成“元”，数据库也保存整数。

## 2. 什么是值对象

值对象由它包含的值决定身份。两个 Money 只要分数相同，就被认为相等，不需要
UUID。

Money 将提供：

- 从“分”明确创建对象。
- 读取分数。
- 判断正数、负数和零。
- 金额加法和减法。
- 按值比较是否相等。

`Money::fromCents(100)` 比 `Money(100)` 更清楚，因为调用者能直接看到单位。

## 3. 本课完成后的目录

```text
src/features/accounting/domain/
  CMakeLists.txt
  Money.h
  Money.cpp

tests/
  CMakeLists.txt
  unit/
    accounting/
      CMakeLists.txt
      MoneyTest.cpp
```

领域库名称为：

```text
StuKitAccountingDomain
```

测试可执行目标名称为：

```text
StuKitMoneyTest
```

这也是 [ADR 0001](../architecture/decisions/0001-build-target-structure.md) 中规划的
第一次 CMake 目标拆分。

## 4. 创建 Money 头文件

创建 `src/features/accounting/domain/Money.h`：

```cpp
#pragma once

#include <QtGlobal>

namespace stukit::accounting {

class Money final
{
public:
    static Money fromCents(qint64 cents) noexcept;

    [[nodiscard]] qint64 cents() const noexcept;
    [[nodiscard]] bool isZero() const noexcept;
    [[nodiscard]] bool isPositive() const noexcept;
    [[nodiscard]] bool isNegative() const noexcept;

    [[nodiscard]] Money operator+(const Money &other) const noexcept;
    [[nodiscard]] Money operator-(const Money &other) const noexcept;

    friend bool operator==(const Money &left, const Money &right) = default;

private:
    explicit Money(qint64 cents) noexcept;

    qint64 m_cents;
};

} // namespace stukit::accounting
```

关键点：

- `#pragma once` 防止头文件被重复包含。
- `final` 表示 Money 不作为继承基类使用。
- `[[nodiscard]]` 提醒调用者不要无意丢弃返回结果。
- `noexcept` 表示这些操作不会抛出异常。
- 构造函数私有，调用者必须使用单位明确的 `fromCents`。
- 默认等号比较会逐个比较成员，本类只有 `m_cents`。

本课允许 Money 为负数，因为月度结余和金额差可能为负数。以后创建支出或收入
账单时，再由 Transaction 业务规则要求录入金额必须大于零。

## 5. 实现 Money

创建 `src/features/accounting/domain/Money.cpp`：

```cpp
#include "Money.h"

namespace stukit::accounting {

Money Money::fromCents(qint64 cents) noexcept
{
    return Money(cents);
}

qint64 Money::cents() const noexcept
{
    return m_cents;
}

bool Money::isZero() const noexcept
{
    return m_cents == 0;
}

bool Money::isPositive() const noexcept
{
    return m_cents > 0;
}

bool Money::isNegative() const noexcept
{
    return m_cents < 0;
}

Money Money::operator+(const Money &other) const noexcept
{
    return Money(m_cents + other.m_cents);
}

Money Money::operator-(const Money &other) const noexcept
{
    return Money(m_cents - other.m_cents);
}

Money::Money(qint64 cents) noexcept
    : m_cents(cents)
{
}

} // namespace stukit::accounting
```

头文件说明“能做什么”，实现文件说明“如何完成”。调用者只需要包含 Money.h，
不需要了解实现细节。

整数极限溢出处理暂不属于本课范围。以后加入输入限制和导入功能时再建立金额
上限规则。

## 6. 创建领域库 CMake 目标

创建 `src/features/accounting/domain/CMakeLists.txt`：

```cmake
add_library(StuKitAccountingDomain STATIC
    Money.cpp
    Money.h
)

target_include_directories(StuKitAccountingDomain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingDomain
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitAccountingDomain
    PUBLIC
        cxx_std_20
)
```

这里使用普通 `add_library`，因为 Domain 不需要 QML、资源系统或其他 Qt 特殊
构建步骤。

依赖使用 `PUBLIC` 的含义是：

- Money 自己需要 Qt Core 的 `qint64`。
- 使用 Money 的目标也能获得必要的包含路径和 Qt Core 依赖。

## 7. 在根 CMake 中启用领域库和测试

在根 CMakeLists.txt 的 `qt_standard_project_setup` 后增加：

```cmake
include(CTest)

add_subdirectory(src/features/accounting/domain)

if(BUILD_TESTING)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
```

不要删除原有 `find_package(Qt6 ... Quick)`。Qt Test 只在 `BUILD_TESTING` 开启时
查找，发布应用本身不会链接测试库。

`include(CTest)` 会创建 `BUILD_TESTING` 选项并默认开启测试。

如果通过 Qt Creator 创建 C++、测试或 CMake 文件，它可能把这些文件自动追加到
现有 `qt_add_qml_module` 的 `SOURCES` 或 `RESOURCES` 中。请删除这些自动追加
行：Money 属于 StuKitAccountingDomain，MoneyTest 属于 StuKitMoneyTest，
它们不能再次作为 StuKit QML 模块的资源或源码注册。

## 8. 创建测试目录入口

创建 `tests/CMakeLists.txt`：

```cmake
add_subdirectory(unit/accounting)
```

它的作用是继续进入记账单元测试目录。以后可以并列增加 schedule、shared 和
integration 等测试目录。

## 9. 创建测试目标

创建 `tests/unit/accounting/CMakeLists.txt`：

```cmake
add_executable(StuKitMoneyTest
    MoneyTest.cpp
)

target_link_libraries(StuKitMoneyTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.money
    COMMAND StuKitMoneyTest
)
```

`StuKitMoneyTest` 是一个普通命令行程序。`add_test` 将它注册到 CTest，测试名称
使用 `模块.对象` 形式，方便以后筛选。

## 10. 编写 Money 单元测试

创建 `tests/unit/accounting/MoneyTest.cpp`：

```cpp
#include <QtTest>

#include "Money.h"

using stukit::accounting::Money;

class MoneyTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsFromCents();
    void identifiesSign();
    void addsValues();
    void subtractsValues();
    void comparesByValue();
};

void MoneyTest::createsFromCents()
{
    const Money money = Money::fromCents(12345);

    QCOMPARE(money.cents(), qint64{12345});
}

void MoneyTest::identifiesSign()
{
    const Money positive = Money::fromCents(1);
    const Money zero = Money::fromCents(0);
    const Money negative = Money::fromCents(-1);

    QVERIFY(positive.isPositive());
    QVERIFY(!positive.isZero());
    QVERIFY(!positive.isNegative());

    QVERIFY(zero.isZero());
    QVERIFY(!zero.isPositive());
    QVERIFY(!zero.isNegative());

    QVERIFY(negative.isNegative());
    QVERIFY(!negative.isZero());
    QVERIFY(!negative.isPositive());
}

void MoneyTest::addsValues()
{
    const Money result = Money::fromCents(120) + Money::fromCents(30);

    QCOMPARE(result.cents(), qint64{150});
}

void MoneyTest::subtractsValues()
{
    const Money result = Money::fromCents(120) - Money::fromCents(150);

    QCOMPARE(result.cents(), qint64{-30});
}

void MoneyTest::comparesByValue()
{
    QVERIFY(Money::fromCents(500) == Money::fromCents(500));
    QVERIFY(Money::fromCents(500) != Money::fromCents(501));
}

QTEST_APPLESS_MAIN(MoneyTest)

#include "MoneyTest.moc"
```

每个测试只验证一个行为。测试名称应描述行为，而不是使用 `test1`、`test2`。

`QTEST_APPLESS_MAIN` 创建不需要 GUI 的测试入口，因此测试 Domain 时不会打开
窗口。

## 11. 重新配置和构建

因为增加了新的 CMakeLists.txt 和目标，需要先在 Qt Creator 中运行 CMake，
然后构建整个项目。

在构建输出中应该出现：

```text
StuKitAccountingDomain
StuKitMoneyTest
StuKit
```

如果 Qt Creator 只构建 StuKit，可以在“项目 -> 构建设置”中确认默认构建目标
为 `all`，或者单独选择构建 StuKitMoneyTest。

## 12. 运行测试

优先使用 Qt Creator 的 Tests 面板运行 `accounting.money`。也可以在配置了 Qt
环境的终端中使用：

```powershell
ctest --test-dir build/Desktop_Qt_6_10_1_MSVC2022_64bit-Debug `
      --output-on-failure
```

Git Bash 中路径分隔符和换行方式不同，第一次建议直接使用 Qt Creator 的 Tests
面板，减少环境变量干扰。

成功结果应为：

```text
100% tests passed, 0 tests failed out of 1
```

## 13. 验证测试确实有效

临时将 `addsValues` 中的期望值从 `150` 改为 `151`，重新运行测试，确认它会
失败并指出实际值与期望值不同。随后立即恢复为 `150`，再次确认测试通过。

这个步骤证明测试不是“只被编译但没有执行”。最终代码中必须保留正确期望值。

## 14. 架构结果

本课结束后的依赖关系：

```text
StuKitMoneyTest
       |
       v
StuKitAccountingDomain
       |
       v
   Qt6::Core
```

Domain 不依赖 QML、Qt Quick、数据库或应用窗口。测试也不需要启动 StuKit。

应用目标暂时没有使用 Money，所以先不链接领域库。下一步实现 Transaction 时，
再通过 Application/ViewModel 将领域对象接入界面。

## 15. 常见错误

### Cannot open include file: Money.h

检查领域库的 `target_include_directories` 是否为 `PUBLIC`，测试目标是否链接了
StuKitAccountingDomain。

### Unknown CMake command add_test

检查根 CMakeLists.txt 中是否调用了 `include(CTest)`。

### Qt6::Test target was not found

检查 `find_package(Qt6 ... COMPONENTS Test)` 是否位于添加 tests 子目录之前，
并确认当前 Qt 安装包含 Qt Test。

如果 QtTest 已安装但 `#include <QtTest>` 在编辑器中标红，先检查 MoneyTest.cpp
是否只属于 StuKitMoneyTest 目标，再重新运行 CMake，让 Qt Creator 刷新代码
模型。不要把测试文件加入 `qt_add_qml_module` 来消除红线。

### Qt Creator 把测试文件加入 qt_add_qml_module

从 `qt_add_qml_module` 中删除自动生成的 `SOURCES` 和 `RESOURCES` 行。该模块的
`QML_FILES` 部分应继续只包含 `${STUKIT_QML_FILES}`。C++ 领域文件和测试文件
分别由它们自己的子目录 CMakeLists.txt 管理。

### vtable 或 MoneyTest.moc 相关错误

检查类中是否有 `Q_OBJECT`，文件末尾是否包含 `#include "MoneyTest.moc"`，并
重新运行 CMake。

### CTest 找不到 DLL

优先从 Qt Creator Tests 面板运行。如果普通终端没有 Qt 运行环境，测试程序
可能找不到 Qt6Core 或 Qt6Test DLL，这不是 Money 代码错误。

## 16. 本课验收清单

- [x] Money 使用 qint64 保存分，不使用 double。
- [x] Money 位于独立的 StuKitAccountingDomain 静态库。
- [x] Domain 不依赖 QML、Qt Quick 或 Qt SQL。
- [x] StuKitMoneyTest 能够单独构建。
- [x] accounting.money 已注册到 CTest。
- [x] 五个测试函数全部通过。
- [x] 临时错误期望值能够让测试失败。
- [x] 恢复正确期望值后测试再次通过。
- [x] StuKit 应用仍能正常构建和运行。
- [x] Git 状态中没有构建产物。

## 17. 完成后反馈

请提供：

```text
CMake 配置：成功 / 失败
StuKitAccountingDomain 构建：成功 / 失败
StuKitMoneyTest 构建：成功 / 失败
accounting.money：通过 / 失败
故意改错后的测试：按预期失败 / 没有失败
恢复后测试：通过 / 失败
StuKit 应用：正常 / 异常
测试输出：
git diff --stat 输出：
```

## 18. 完成记录

完成日期：2026-10-07。

验收结果：

```text
CMake 配置：成功
StuKitAccountingDomain 构建：成功
StuKitMoneyTest 构建：成功
accounting.money：通过
测试结果：100% tests passed, 0 tests failed out of 1
故意修改期望值：测试按预期失败
恢复正确期望值：测试重新通过
StuKit 应用：正常
```

普通终端直接运行 CTest 时需要让 Qt 运行目录出现在 PATH 中；Qt Creator Tests
面板会使用 Kit 环境。Money 领域库和测试目标已经实现 ADR 0001 规划的第一次
CMake 目标拆分。
