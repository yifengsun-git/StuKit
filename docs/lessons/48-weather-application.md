# 第四十八课：天气异步 Application 端口

## 本课目标

本课定义天气模块如何发起城市搜索和预报查询，但仍不关心具体 HTTP URL 与 JSON。
重点不是“发请求”本身，而是保证快速连续操作时只有最新请求可以改变页面。

## 1. 稳定的服务结果

`WeatherServiceTypes.h` 定义两类结果：

- `LocationSearchResult`：地点列表或稳定错误；
- `ForecastResult`：合法 `WeatherForecast` 或稳定错误。

错误分为输入错误、网络不可用、接口限流、服务不可用、响应损坏和取消。Presentation
以后只需把这些类型映射为用户提示，不解析 Qt Network 的底层错误码。

## 2. 两个基础设施端口

- `ILocationSearchService`：接收搜索文本，异步返回候选地点。
- `IWeatherForecastService`：接收已经验证的地点，异步返回预报。

端口使用回调并返回请求 ID，同时提供 `cancel()`。接口约定实现必须异步、最多回调一次。
这样 Open-Meteo 实现、离线假实现和自动测试替身都能使用同一套用例。

## 3. 为什么要有 generation

网络响应不保证按发送顺序返回。例如：

1. 用户搜索“上海”，请求 A 发出。
2. 用户立即改为“北京”，请求 B 发出并取消 A。
3. A 的响应已经在途中，仍可能晚于取消到达。

`SearchWeatherLocations` 和 `GetWeatherForecast` 每次执行都增加 generation。回调携带
发出时的 generation，只有仍等于当前值才会向上交付。取消既通知基础设施停止网络，
也立即使旧回调失效，因此旧结果无法覆盖新结果。

共享状态由 `shared_ptr` 持有。即使用例析构后底层才完成取消，回调也只会读取失效的
generation，不会访问已经销毁的 `this`。

## 4. 输入边界

城市关键词会先去除首尾空白，长度必须为 2～100 个字符。非法输入直接返回
`InvalidInput`，不会消耗一次网络请求。

## 5. 验收点

- 城市关键词会规范化后再转交服务。
- 非法关键词不调用供应商。
- 新搜索会取消旧搜索，旧回调被丢弃。
- 新预报请求会取消旧预报，旧错误同样不会污染新状态。
- 服务启动失败会映射为稳定的 `ServiceUnavailable`。
- `weather.domain` 与 `weather.application` 测试均通过。

下一课实现 Open-Meteo 基础设施：集中构造 URL、解析固定 JSON 样例，再将 Qt Network
错误映射到本课定义的稳定错误。
