# 第 77 课：模型配置与 Windows 安全密钥

## 本课目标

- 用统一端口隔离规划业务与具体模型服务。
- 保存 API 基础地址和模型名称，但不明文保存 API Key。
- 支持 DeepSeek 默认配置以及其他 OpenAI Chat Completions 兼容服务。
- 在发送资料前明确告知用户数据去向。

## 两类配置分开保存

基础地址和模型名称不是秘密，保存在 StuKit 的设置文件中。API Key 属于凭据，通过
`ISecretStore` 写入 Windows Credential Manager，凭据名称为 StuKit 自己的命名空间。
数据库、`settings.ini`、日志和 Markdown 导出都不会包含密钥。

删除密钥按钮会从凭据管理器移除该项。设置弹窗中留空 API Key 表示继续使用现有密钥，
避免界面为了回显而把密钥重新读出来。

## 可替换端口

- `ILanguageModelService`：接收与厂商无关的请求并异步返回 Markdown 或错误；
- `ISecretStore`：读、写、删除命名凭据；
- `OpenAiCompatibleLanguageModelService`：首个 HTTP 适配器；
- `WindowsCredentialSecretStore`：Windows 首版安全存储实现。

这使得后续增加 Anthropic Messages 或 OpenAI Responses 时，不需要改动资料 Repository。
Android 暂不实现；以后只需为 `ISecretStore` 增加 Android Keystore 适配器。

## 安全边界

- API 地址必须使用 HTTPS；
- 请求与授权头不写日志；
- 导入资料始终视为不可信数据，资料里的“忽略系统提示”等文字不能取得指令权限；
- 每次生成前弹框显示服务地址、模型和将发送的资料数量；
- 原文件不上传，上传的是用户已审阅的数据库文本副本。

## 下一课

第 78 课完成一次真正的规划生成：构建受控上下文、处理网络错误、保存模型来源和资料
快照，并把结果作为新的 Markdown 版本交给用户继续编辑。
