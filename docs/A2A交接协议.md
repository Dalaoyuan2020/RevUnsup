# A2A (Agent-to-Agent) 交接协议

> **L4 连接层文档** · 强制规则
> 任何 Agent 在 RevUnsup 项目中工作前必须读这份协议
> 创建日期: 2026-04-09 (源自一次 DogWind 误删 6 个文件的事故)

---

## 为什么需要这个协议

### 事故记录: 2026-04-09

DogWind 在执行 RU-002+RU-003 时, 一次 push 净删了 1068 行, 删掉了 6 个属于其他 Agent 工作范围的文件:
- OldMajor_Codex 的整个人格 (PERSONA.md + skills/)
- OM-001 任务文档
- OldMajor_Codex 写的预实验经验清单
- Napoleon (Claude) 写的编排路线和设备地图

**根因诊断**:
- DogWind 大概率基于旧版本 (RU-001 完成时) 工作, 没 git pull 最新
- DogWind 不知道在它工作期间, OldMajor_Codex 推了一份完整的 OM-001
- DogWind 不知道 Napoleon 推了编排路线和设备地图
- DogWind 提交时强行覆盖, 把不知道的文件全删了

**真正的问题**: 不是 GitHub 慢, 不是 SSH 没搭, 是【Agent 之间没有交接协议】, 多个 Agent 像三只手抢一个键盘。

---

## 协议核心: 三段式

```
事前 (开工前)        事中 (工作中)         事后 (提交前)
   ↓                    ↓                     ↓
  PULL                BOUNDARY                DIFF
  READ                NO-CROSS                DELETE-CHECK
  REPORT              ASK-IF-UNSURE           HANDOVER
```

---

## 一、事前协议 (开工前必做)

### Step 1.1: 强制 git pull
```bash
cd <RevUnsup 路径>
git pull --ff-only  # 失败立即停止, 不要 force
```

如果 `--ff-only` 失败, 说明本地有冲突, 立即报告人类, **不要自己解决**。

### Step 1.2: 加载人格和技能
- 读 `.persona/<我的Agent名>/PERSONA.md`
- 扫 `.persona/<我的Agent名>/skills/` 全部
- 确认我是谁、我的边界在哪

### Step 1.3: 阅读全局上下文
**不只是看自己的任务**, 还要看:
- `docs/编排路线-v*.md` (最新版)
- `docs/设备地图.md`
- `docs/A2A交接协议.md` (本文件)
- 其他 Agent 最近交付的产出 (在 `docs/` 目录里)

### Step 1.4: 报告当前看到的状态
向人类汇报:
```
我是 <Agent名>, 启动于 <时间>
当前 main HEAD: <commit-hash>
我看到其他 Agent 最近的产出:
  - <产出1>: <文件路径>
  - <产出2>: <文件路径>
我即将开始任务: <任务编号>
我的工作 scope: <文件/目录列表>
我承诺不动的: <文件/目录列表>
```

人类回复 OK 后才能进入工作阶段。

---

## 二、事中协议 (工作中)

### 边界规则 (Scope)

每个 Agent 有自己的【可写区】和【只读区】:

| Agent | 可写区 | 只读区 |
|---|---|---|
| **DogWind** (XYC_Windsurf) | `docs/封装版*` `docs/杠杆*` `docs/阈值*` `docs/baseline*` `tasks/RU-*.md` `src/` `deploy/` | 其他全部 |
| **OldMajor_Codex** | `docs/预实验*` `docs/旧版*` `tasks/OM-*.md` | 其他全部 |
| **Napoleon** (Claude) | `docs/编排*` `docs/设备*` `docs/A2A*` `.persona/` `dashboard/` `configs/lighthouse.yaml` | `tasks/RU-*` `tasks/OM-*` (只读, 由对应 Agent 维护) |

### 三条铁律

1. **只在自己的可写区动手** — 不知道某个文件该谁动, 就不动
2. **不删除自己没创建的文件** — 看到不认识的文件 → 假设它有用, 留着
3. **跨 scope 的修改 → 暂停 → 报告 → 等批准** — 不要先斩后奏

### 不确定时的默认动作

```
看到不认识的文件 → 留着, 不删
看到不熟悉的目录 → 不进
看到内容冲突 → 报告, 不合并
需要修改其他 Agent 的文件 → 报告, 等批准
```

---

## 三、事后协议 (提交前必做)

### Step 3.1: 强制二次 pull
工作完成后再 pull 一次, 看看有没有人在我工作期间也推了东西:
```bash
git pull --ff-only
```

如果有冲突, 立即停止, 报告人类。

### Step 3.2: 列出本次修改清单
```bash
git status
git diff --stat HEAD
```

把【add / modify / delete】三类清单写出来。

### Step 3.3: 删除文件检查 (最重要)

**任何 delete 必须满足以下三个条件之一**:
1. 这个文件是我自己创建的
2. 我有人类的明确指令删除它
3. 这个文件在我的可写区内, 且我有合理理由

**如果不满足任何一条 → 立即 abort, 不要 push**, 报告人类决定。

### Step 3.4: commit message 格式

```
<类型>(<范围>): <简短描述>

我做了:
- <动作1>
- <动作2>

我没碰:
- <声明 1>
- <声明 2>

我删除了:
- <文件1> (理由)
- <文件2> (理由)

下一个 Agent 注意:
- <如有>
```

### Step 3.5: push
```bash
git push  # 不加 --force
```

如果 push 被拒, 立即停止, 报告人类。**绝不 force push**。

---

## 四、交接文档 (Handover)

完成一个任务后, 写一份 `HANDOVER_<任务编号>.md` 放在 `tasks/handovers/` 目录:

```markdown
# Handover: <任务编号>

执行人: <Agent名>
完成日期: <日期>
commit: <hash>

## 我做了什么
- ...

## 我交付了什么
- <文件1>: <说明>
- <文件2>: <说明>

## 我没碰的东西 (重要)
- <声明>

## 下一个 Agent 要注意的
- 前置依赖
- 已知陷阱
- 我留下的 TODO

## 我看不到/没解决的
- ...
```

---

## 五、scope 冲突的处理

如果两个 Agent 都需要修改同一个文件, 走以下流程:

1. **第一个 Agent 启动时**, 在 `tasks/locks/<文件名>.lock` 写一个简单的 lock 文件
2. **第二个 Agent 启动时检查 lock**, 如果存在 → 报告 → 等待
3. **第一个 Agent 完成后**, 删除自己的 lock
4. lock 文件内容:
```
agent: <Agent名>
acquired_at: <时间>
file: <被锁的文件路径>
expected_release: <预计完成时间>
```

⚠️ 这个 lock 是【荣誉锁】, 不是真正的并发锁。它的作用是让人类能看到谁在做什么。

---

## 六、紧急情况: 协议被违反时

如果一个 Agent 违反了协议 (比如 DogWind 这次的事故), 走以下流程:

1. **立即停止该 Agent 的后续工作**
2. **保留它的高价值产出** (不要全 revert)
3. **从 git history 恢复被误删的文件**
4. **写一份 INCIDENT_<日期>.md** 记录事故和教训
5. **更新本协议** 加入新的防护规则

---

## 七、本协议的扩展(灯塔, 不实现)

按反过度工程化原则, 以下扩展先记录, 不动手:

- **真正的文件锁** (使用 git LFS lock 或类似)
- **CI 检查 commit 是否符合协议** (GitHub Actions)
- **Agent 之间的直接消息通道** (走 Relay API, 见 lighthouse.yaml dashboard_communication)
- **scope 自动校验** (push 前 hook 检查)

---

## 八、Agent 速查清单 (打印贴在脑门上)

```
开工前:
  [ ] git pull --ff-only
  [ ] 读 PERSONA.md + skills/
  [ ] 读 编排路线 + 设备地图 + A2A 协议
  [ ] 报告"我看到的状态"给人类
  [ ] 等批准

工作中:
  [ ] 只在我的可写区动手
  [ ] 不删别人的文件
  [ ] 不动别人的人格
  [ ] 不确定就停, 不要猜

提交前:
  [ ] git pull --ff-only (二次同步)
  [ ] 列 add/modify/delete 清单
  [ ] 检查 delete: 是不是自己创建的?
  [ ] commit message 包含"我没碰什么"
  [ ] git push (不加 --force)

完成后:
  [ ] 写 HANDOVER_<任务编号>.md
  [ ] 报告人类: 任务完成 + 下一步建议
```

---

*v1.0 / 2026-04-09 / 源于 DogWind 删 6 文件事故*
*这份协议存在的意义是: 让事故只发生一次*
