# 第 83 课：多模型服务与初版验收

## 本课目标

- 让 DeepSeek、OpenAI、Grok 共用 OpenAI Chat Completions 兼容适配器。
- 为 Claude 增加独立的 Anthropic Messages 适配器。
- 不让请求协议差异渗透到规划业务。
- 完成 Windows 初版的构建和回归验收。

## 路由结构

```text
PlanningViewModel
    -> ILanguageModelService
    -> RoutingLanguageModelService
         |-> OpenAiCompatibleLanguageModelService
         |     DeepSeek / OpenAI / xAI / 自定义网关
         |-> AnthropicLanguageModelService
               Claude Messages API
```

OpenAI 兼容请求使用 `Authorization: Bearer` 和 `/chat/completions`。Claude 请求
使用 `/v1/messages`、`Authorization: Bearer` 和 `anthropic-version: 2026-10-21`。
两个适配器都转换成统一的 `LanguageModelResult`。
对未绑定单一工作区的 Anthropic Key，设置页还可以填写可选的
`anthropic-workspace-id`。

## 凭据隔离

OpenAI 兼容协议和 Anthropic 协议使用不同的 Windows 凭据名称。切换协议时，
界面会检查目标协议是否已有密钥，不会把 Claude Key 误发给 OpenAI 兼容地址，
也不会把兼容接口 Key 误发给 Anthropic。

`settings.ini` 只保存协议类型、基础地址和模型名称，不保存 API Key。

## 界面预设

模型设置提供 DeepSeek、OpenAI、Claude 和 Grok 快速预设，同时允许手动编辑地址和
模型 ID。预设只是填表辅助；最终可用模型取决于用户账户的权限和服务商现状。

## 初版验收结果

- Windows MSVC Debug 主程序和 QML 预编译通过。
- 规划 Domain、Workspace 和 SQLite Repository 测试通过。
- 测试覆盖项目和资料、事实抽取、输出门禁、机会评分、项目编辑、
  Markdown 行动发布和 Anthropic 协议路由。
- 没有 API Key 时，本地项目、资料、评分和 Markdown 版本仍然可用。
- 仍需用户自己的有效 API Key 执行一次真实服务请求验收；自动测试不会联网消耗额度。

## 后续而非初版阻塞项

- 原生 OpenAI Responses 和 xAI Responses 适配器。
- PDF / DOCX / OCR 导入。
- 课表不可用时间和记账预算摘要的只读联动。
- 计划版本对比、行动来源链接表和去重发布。
- Android 界面、移动端安全存储和云同步。
