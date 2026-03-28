# Agent 上手指南

你是一个 AI Agent，被分配到 RevUnsup 项目中执行任务。请按以下步骤设置环境、读取任务、开始工作。

---

## 第一步：克隆仓库

```bash
git clone https://github.com/Dalaoyuan2020/RevUnsup.git
cd RevUnsup
```

## 第二步：了解项目

必读文件（按顺序）：

| 文件 | 内容 |
|---|---|
| `README.md` | 项目概览、技术栈、结构 |
| `docs/ARCHITECTURE_REVERSE_ENGINEERING.md` | 逆向分析文档，理解企业原版做了什么 |
| `docs/AGENT_TASKS.md` | 三个 Agent 的详细任务分工和 Prompt |
| `configs/patchcore_default.yaml` | 核心算法参数，不要修改 |

## 第三步：确认你的 Agent 角色

| Agent | 负责人 | 核心任务 |
|---|---|---|
| Agent 1 | Codex | Python 训练流程（ROI裁切 + 数据集 + 训练 + 阈值） |
| Agent 2 | Claude Code | 推理入口 + ONNX 导出（⚠️ 最难的部分） |
| Agent 3 | OpenCode | C++ 推理引擎 + DLL 封装 + C# 界面 |

找到 `docs/AGENT_TASKS.md` 中你对应的 Prompt，严格按照要求执行。

## 第四步：配置密钥（本地操作，不推到远程）

```bash
cp .env.example .env
# 然后编辑 .env，填入真实密钥
```

## 第五步：连接 Notion 看板

项目看板用 Notion 管理，通过 API 读取你的任务状态。

### 连接方式

```python
import requests
import os

NOTION_TOKEN = os.getenv("NOTION_TOKEN")  # 从 .env 读取

headers = {
    "Authorization": f"Bearer {NOTION_TOKEN}",
    "Notion-Version": "2022-06-28"
}
```

### 读取看板任务列表

```python
DATABASE_ID = "从看板URL中提取的32位ID"

# 查询所有任务卡片
r = requests.post(
    f"https://api.notion.com/v1/databases/{DATABASE_ID}/query",
    headers={**headers, "Content-Type": "application/json"},
    json={}
)

if r.status_code == 200:
    tasks = r.json()["results"]
    for task in tasks:
        props = task["properties"]
        title = props.get("Name", {}).get("title", [{}])[0].get("plain_text", "无标题")
        status = props.get("Status", {}).get("status", {}).get("name", "未知")
        agent = props.get("负责 Agent", {}).get("select", {}).get("name", "未分配")
        print(f"[{status}] {title} — {agent}")
else:
    print(f"❌ 读取失败: {r.status_code} {r.json().get('message', '')}")
```

### 更新任务状态

```python
PAGE_ID = "任务卡片的ID"

# 把任务改为"进行中"
r = requests.patch(
    f"https://api.notion.com/v1/pages/{PAGE_ID}",
    headers={**headers, "Content-Type": "application/json"},
    json={
        "properties": {
            "Status": {
                "status": {"name": "In Progress"}
            }
        }
    }
)

if r.status_code == 200:
    print("✅ 任务状态已更新")
else:
    print(f"❌ 更新失败: {r.status_code}")
```

### 验证连接

```python
# 验证 token 是否有效
r = requests.get(
    "https://api.notion.com/v1/users/me",
    headers=headers
)

if r.status_code == 200:
    print("✅ Notion 连接成功！")
    print(f"  用户: {r.json().get('name', '未知')}")
else:
    print("❌ 连接失败，检查 NOTION_TOKEN 是否正确")
```

## 第六步：GitHub 工作流

### 拉取最新代码
```bash
git pull origin main
```

### 创建你的工作分支
```bash
# Agent 1
git checkout -b agent1/python-training

# Agent 2
git checkout -b agent2/inference-export

# Agent 3
git checkout -b agent3/cpp-deployment
```

### 提交你的工作
```bash
git add <你修改的文件>
git commit -m "feat(agent1): 实现 ROI 对齐裁切模块"
git push origin <你的分支名>
```

### 提交 PR
```bash
# 完成所有任务后，提交 PR 到 main 分支
gh pr create --title "Agent 1: Python 训练流程完成" --body "完成了5个脚本 + LEARN_AGENT1.md"
```

## 第七步：交付检查清单

完成任务后，对照检查：

### Agent 1 (Codex)
- [ ] src/roi_alignment.py 所有函数已实现
- [ ] 5 个 scripts 脚本可独立运行
- [ ] 每个函数有中文注释
- [ ] 每个脚本有验证输出（print ✅）
- [ ] 生成 docs/LEARN_AGENT1.md
- [ ] model.ckpt 已生成
- [ ] thresholds.json 已生成

### Agent 2 (Claude Code)
- [ ] src/infer_service.py 所有方法已实现
- [ ] src/export_onnx.py 拆分导出完成
- [ ] feature_extractor.onnx 已导出
- [ ] memory_bank.npy 已导出
- [ ] deploy_config.json 已导出
- [ ] Python推理 vs ONNX推理 分数一致（差值 < 0.001）
- [ ] 生成 docs/LEARN_AGENT2.md

### Agent 3 (OpenCode)
- [ ] patchcore_engine.cpp 编译通过
- [ ] anomalyDet.dll 生成成功
- [ ] C# 界面可运行
- [ ] RTX 2080 推理延迟 ≤100ms
- [ ] 耗时分步统计（特征提取/KNN/后处理）
- [ ] 生成 docs/LEARN_AGENT3.md

---

## 看板链接

⚠️ 以下链接仅供项目内部使用：

- **Notion 项目主页**: 见 .env 中的 NOTION_BOARD_URL
- **Notion 看板视图**: 登录后在主页切换到 Board View

## 遇到问题？

1. 先读 `docs/ARCHITECTURE_REVERSE_ENGINEERING.md` 确认你理解原版架构
2. 再读 `docs/AGENT_TASKS.md` 确认任务要求
3. 检查 `configs/patchcore_default.yaml` 参数是否对齐
4. 如果是 Agent 间交接问题，检查上游 Agent 的交付物格式
