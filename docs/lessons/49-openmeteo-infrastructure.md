# 第四十九课：Open-Meteo 网络与 JSON 基础设施

## 本课目标

本课让天气 Application 端口拥有第一个真实实现：使用 Qt Network 访问 Open-Meteo，
集中构造 URL、解析城市和预报 JSON，并把底层故障转换为稳定错误。

## 1. URL 构造器

`OpenMeteoUrlBuilder` 负责所有供应商参数：

- 城市搜索固定返回最多 8 个中文结果；
- 预报固定请求摄氏度、km/h 和毫米；
- 当前天气只请求页面需要的温度、体感、湿度、降水、天气代码和风况；
- 逐日预报请求 7 天最高/最低温、天气代码、降水概率、降水量与日出日落；
- `timezone` 使用所选城市的 IANA 时区。

ViewModel 和 QML 不拼查询字符串。将来参数或供应商变化只修改 Infrastructure。

## 2. JSON 解析器

`OpenMeteoJsonParser` 先检查 JSON 根结构、必需字段类型和所有逐日数组长度，再调用
Domain 工厂做第二层业务校验。

Open-Meteo 在指定 `timezone` 后返回不含偏移的城市本地时间，例如
`2026-10-13T10:00`。解析器不能让 `QDateTime` 使用电脑本地时区，而是显式使用
`WeatherLocation.timezoneId` 构造时间。否则查看国外城市时，观测时间、日出和日落会
整体错位。

地点搜索的合法空结果返回空列表；损坏 JSON、缺字段、错误类型和预报数组长度不一致
返回 `InvalidResponse`。

## 3. Qt Network 服务

`OpenMeteoWeatherService` 同时实现地点搜索和预报服务：

- 使用单个 `QNetworkAccessManager` 复用连接；
- 每次请求生成唯一 ID，供 Application 取消旧请求；
- 15 秒无传输后超时；
- HTTP 429 映射为 `RateLimited`；
- HTTP 5xx 映射为 `ServiceUnavailable`；
- 取消映射为 `Cancelled`，其他传输故障映射为 `NetworkUnavailable`；
- `QNetworkReply` 完成后统一 `deleteLater()`，不泄漏网络对象。

测试不会访问公网。URL 和 JSON 使用固定样例验证，因此在离线环境也能稳定执行。真实
端到端联网只在应用运行时发生。

## 4. 数据来源边界

当前个人非商业开发使用 Open-Meteo 免费接口，无需 API Key。免费接口有调用限制和
无可用性保证，天气数据要求 CC BY 4.0 署名。后续天气页面必须显示 Open-Meteo 来源；
如果应用加入广告或收费，需要在发布前确认商业许可。

## 5. 验收点

- 查询文本和坐标通过 `QUrlQuery` 编码，不手工拼接。
- 请求明确城市时区与单位。
- 城市、当前天气和逐日数据可从固定样例恢复为 Domain 对象。
- 日出、日落和观测时间属于目标城市时区。
- 损坏 JSON 和错位数组被拒绝。
- 取消、限流、服务错误和网络错误使用稳定错误类型。
- 三项天气测试均可离线通过。

下一课进入 Presentation 与组合根：将城市候选、加载状态、用户提示和预报列表暴露给
QML，并保存用户选中的城市。
