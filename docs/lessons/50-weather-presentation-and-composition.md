# 第五十课：天气 Presentation 与应用组合根

## 1. 本课目标

上一课已经能把 Open-Meteo 响应转换成领域对象，但 QML 仍不能直接使用这些异步用例。
本课增加 `WeatherViewModel`，并由 `WeatherModule` 在应用入口集中创建网络、基础设施、
用例和展示层对象。天气模块至此形成从真实服务到界面边界的完整 C++ 链路。

本课暂不绘制天气页面；第 51 课再让 QML 消费这些属性和操作。

## 2. WeatherViewModel 的职责

`WeatherViewModel` 只面向界面暴露容易使用的状态：

- `searching`、`loading`：区分城市搜索和天气刷新；
- `searchResults`：地点名称、行政区、国家和时区；
- `locationName`、`locationSubtitle`：当前选中的城市；
- 当前温度、体感温度、天气、湿度、降水和风况的格式化文本；
- `dailyForecasts`：QML 可直接遍历的逐日预报；
- `hasError`、`errorMessage`：稳定的中文错误信息。

界面只需调用 `search(query)`、`selectLocation(index)` 和 `refresh()`，不需要拼接 URL、
理解 Open-Meteo 字段或管理异步请求编号。

## 3. 城市选择与启动恢复

选中地点后，ViewModel 把名称、国家、行政区、IANA 时区和经纬度保存到 QSettings 的
`weather/location/*` 键。下次启动时先通过 `WeatherLocation::create` 重新校验这些值；
只有完整且合法的地点才会恢复并自动刷新，损坏的设置不会进入业务对象。

QSettings 只保存用户选择的城市，不保存账户数据库，也不会加入 Git。

## 4. 错误与旧数据

新请求开始时清除旧错误并进入加载状态。请求失败时结束加载、显示可读错误，但不会
删除本次运行中上一次成功取得的预报，因此短暂断网后页面仍可展示已有内容。

这还不是持久化离线缓存。关闭应用后仍能读取最近预报的版本化 JSON 缓存将在第 52 课
实现；当前设计已经为“错误与可用旧数据同时存在”保留了状态。

`WeatherViewModel` 析构时会取消搜索和预报用例。Application 层的 generation 机制继续
阻止慢到达的旧响应覆盖新城市。

## 5. 应用组合根

`WeatherModule` 按依赖顺序持有：

```text
QNetworkAccessManager
  -> OpenMeteoWeatherService
    -> SearchWeatherLocations / GetWeatherForecast
      -> WeatherViewModel
```

这些对象由 `std::unique_ptr` 明确管理生命周期，并按相反顺序销毁。`main.cpp` 只创建
模块，把 `weatherViewModel` 作为初始属性交给 `Main.qml`；QML 不负责创建网络服务或
业务用例。

## 6. 自动化验证

新增的 Presentation 测试使用假搜索服务和假预报服务，不访问公网，覆盖：

- 过短城市名在调用服务前被拒绝；
- 搜索结果能够被选择并触发天气刷新；
- 所选地点写入设置并可在初始化时恢复；
- 成功预报被转换成 QML 文本和逐日列表；
- 刷新失败时保留上一份成功预报。

同时修正了课表 SQLite 测试的临时目录位置：测试目录固定放在测试程序所在的构建输出
目录，不再依赖启动时的当前工作目录，因此不会在仓库源码根目录留下
`stukit-period-definition-test-*` 文件夹。构建目录已被 Git 忽略，已经遗留的四个目录
也已清理。

## 7. 本课完成标准

- 天气 Presentation 只依赖 Application 和 Domain；
- 城市选择能够保存并在启动时恢复；
- 应用入口能向 QML 提供有效的 `weatherViewModel`；
- 天气 Presentation 测试离线通过；
- 完整工程能够构建，全部自动测试和 QML 静态检查通过。

下一课实现响应式天气 UI：城市搜索、当前天气卡片、7 日预报、加载/错误状态和
Open-Meteo 数据署名。
