# 开发协作方式

## 1. 分工

学习者负责：

- 在指导下亲手编写代码。
- 运行构建、测试和应用。
- 提供完整的错误信息和操作结果。
- 在不理解时及时提问，不直接复制无法解释的代码。

Codex 负责：

- 维护项目计划、架构和开发文档。
- 将功能拆成适合新手完成的小任务。
- 解释每一步的目的和相关概念。
- 审查代码、定位错误并给出修改建议。
- 指导测试、Git 提交、Windows 部署和 Android 迁移。

默认情况下，Codex 不直接代写业务代码。需要示例时，会优先给出最小片段并
解释每一部分；学习者完成代码后再进行检查。

## 2. 每轮开发流程

1. 明确本轮唯一目标。
2. 讲解将用到的概念。
3. 给出需要创建或修改的文件清单。
4. 学习者完成代码并构建。
5. 学习者提供代码差异和运行结果。
6. Codex 审查并解释问题。
7. 运行自动测试和人工验收。
8. 更新文档并创建一次清晰的 Git 提交。

## 3. 提问或报告错误时需要提供的信息

- 正在执行哪一个任务。
- 使用的 Qt 版本和编译器 Kit。
- 完整错误信息，不只截取最后一行。
- 修改过的文件或 `git diff`。
- 错误出现前执行的操作。
- 问题能否稳定复现。

如果编译失败，不要连续随意修改多个位置。先保留错误现场，再逐项分析第一条
有效错误。

## 4. Git 使用约定

- main 始终保持可构建。
- 每次提交只完成一个清晰目标。
- 提交前查看 `git diff` 和 `git status`。
- 不提交 build 目录、Qt Creator 用户配置、临时数据库和密钥。
- 不使用强制推送覆盖不确定的远程历史。

建议提交格式：

```text
类型(范围): 简短说明
```

例如：

```text
docs(project): add initial architecture plan
feat(accounting): add transaction entity
test(accounting): cover invalid transaction amount
fix(database): handle failed migration transaction
```

## 5. 文档维护规则

- 功能范围变化时更新 PROJECT_PLAN.md。
- 依赖方向或模块边界变化时更新 ARCHITECTURE.md。
- 新的关键技术选择记录在 docs/architecture/decisions 中。
- 每节课的目标和验收方式记录在 docs/lessons 中。
- README 只保留项目介绍、当前状态和文档入口。
