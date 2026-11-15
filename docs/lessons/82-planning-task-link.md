# 第 82 课：规划行动联动待办

## 本课目标

把规划从“只能阅读的文档”变成可执行的行动，同时保持两个模块的边界：

1. 只解析 Markdown 中未完成的 `- [ ]` 项。
2. 先展示待办草稿列表。
3. 用户确认后才写入待办模块。
4. 创建后在待办中补充截止时间、提醒、优先级和课程关联。

## 为什么不直接写待办表

规划功能不知道待办的 SQLite 表结构，也不依赖其 Repository。它只调用
`IPlanningActionPublisher`。应用组装层的 `TaskPlanningActionPublisher` 把草稿转成
`SaveTaskRequest`，并通过待办自己的用例保存。

```text
Planning Markdown
    -> PlanningActionExtractor
    -> 草稿预览弹窗
    -> 用户确认
    -> IPlanningActionPublisher
    -> SaveTask
    -> 待办列表刷新
```

## 安全和数据规则

- 已完成的 `- [x]` 不会重新创建。
- 同一篇 Markdown 中标题相同的行动会去重。
- 单次最多预览 50 项，标题会限制在待办领域的长度内。
- 资料引用如 `[S1:L2]` 不进入待办标题。
- 新待办使用“规划”分类，如果分类不存在会先由待办模块创建。

## 验收结果

- 没有 `- [ ]` 时不会打开空确认框。
- 取消确认不会修改待办数据。
- 确认后任务立即出现在待办模块。
- 规划模块不包含任何待办 SQL。
