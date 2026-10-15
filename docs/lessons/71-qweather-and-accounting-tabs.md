# 第七十一课：和风天气迁移与记账管理页签优化

## 本课目标

本课完成两个真实使用反馈：把天气数据供应商迁移到和风天气，并把记账“账户与分类
管理”顶部切换改成符合页签直觉的深色选中态。

## 一、天气供应商迁移

天气的 Domain、Application、ViewModel、缓存和页面数据结构保持不变，只替换
Infrastructure 和组合根：

```text
WeatherHomePage
  -> WeatherViewModel
  -> SearchWeatherLocations / GetWeatherForecast
  -> QWeatherWeatherService
     -> GeoAPI 城市搜索
     -> 实时天气 v1 + 每日预报 v1（并行）
```

`QWeatherUrlBuilder` 只接受 HTTPS 专属 Host，坐标按 v1 接口要求保留两位小数；
`QWeatherJsonParser` 将字符串坐标、0～1 湿度/降水概率、m/s 风速和天气代码转换为
StuKit 的稳定领域类型。当前数据没有单独观测时间，因此使用本次抓取时间并转换到地点
IANA 时区。实时和逐日请求只有全部成功才形成新预报，失败时仍保留已有缓存。

认证使用 `X-QW-Api-Key` 请求头。运行时优先读取 `STUKIT_QWEATHER_API_HOST` 与
`STUKIT_QWEATHER_API_KEY`；Android 个人构建可通过本机 CMake Cache 提供回退值。真实
密钥不会写进仓库，具体步骤见 `docs/QWEATHER_SETUP.md`。

## 二、账户与分类页签

“管理全部”弹窗中的账户与分类继续使用 `TabBar` 管理页面索引，但覆盖了默认视觉：

- 当前页签背景为 `#171717`，文字为白色；
- 未选页签使用普通浅色表面；
- 鼠标悬停仍保留轻微反馈；
- 点击分类后，分类变深色、账户恢复未选状态。

这里没有改变账户、分类、归档或恢复的业务逻辑，只修正了选择状态的视觉表达。

## 三、验证

- Qt 6.10.3 MSVC 2022 Debug 完整构建通过。
- 和风天气 URL、Host 安全校验、GeoAPI JSON、实时/逐日 JSON 和时区转换使用离线样例测试。
- QML 集成测试验证账户/分类页签的选中状态和深色背景。
- 自动测试不会访问公网或消耗和风天气额度。
- 本课没有创建 Git 提交。
