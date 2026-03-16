# NetOS 断档恢复文档（Continuity）

> 目标：任何会话重置后，都能**快速恢复项目上下文并继续推进**。本文件为唯一入口。

## TL;DR（最快恢复）
- **工作区**：`/home/ubuntu/Project/NetOS/netos-proto`
- **当前阶段**：Phase 1 + Phase 2.3 + Phase 3.3 + Phase 3.4（Pull 路径 + 传播控制 + 异步转发 + 拓扑热加载），并新增 NS-3 scaffold（文档 + 拓扑导出）。
- **一键验证**：`COMPOSE_BAKE=true ./scripts/demo.sh table-stats --with-health`

---

## 1) 项目定位（1 段话）
NetOS 是一个以“本地优先 + 发现式同步”为核心的分布式缓存同步系统原型，目标是在非 RDMA 低成本网络上接近 RDMA 访问延迟。当前仓库是 **Phase‑1 + Phase‑2.3 Demo**：Redis + 同步进程同容器，UDP REQ/DATA 拉取路径 + Content‑BF/Query‑BF 传播控制，强调可运行、可演示、可迭代。

## 2) 当前状态快照
- ✅ 2 节点 demo：node2 请求 key，node1 响应，node2 落地。
- ✅ 3 节点线性 hop‑through demo（node1 → node2 → node3；Compose + Kathara）。
- ✅ Kathara 4 节点 hub 拓扑 demo（node2 作为 hub）。
- ✅ QueryTable TTL 去重、SyncTable LRU stub 已接入。
- ✅ 统一 demo 驱动与多套验证/叙述脚本。
- ✅ Content-BF 直连未命中时，等待 `NETOS_CONTENT_BF_FALLBACK_MS` 后进入 Query-BF 聚合。
- ✅ Query-BF 命中会对所有本地匹配 key 返回 DATA，同时仍会继续转发。
- ✅ Query-BF 聚合支持 fill ratio / max keys 分裂，并记录 Content/Query-BF 填充率日志。
- ✅ 异步转发：出站消息进入转发队列，默认开启，溢出策略 drop_newest。
- ✅ 动态拓扑热加载：按 `NETOS_TOPOLOGY_RELOAD_MS` 周期重读 env 配置并更新邻居列表。
- ✅ NS-3 scaffold：`docs/NS3.md` + `scripts/ns3/generate_topology.py`（拓扑导出 JSON）。
- ⛔️ **尚未实现**：订阅式 Push、动态拓扑发现/规模测试（Push 阶段仅为 CBF 周期摘要交换：本地 cache -> neighbors，不发送 Interest 包）。

## 3) 核心约束（Phase‑1 Guardrails）
- `req_state=` / `data_state=` 日志标签必须保持稳定（脚本依赖）。
- 暂不引入订阅式 Push / 大规模拓扑（Push 阶段仅为 CBF 周期摘要交换：本地 cache -> neighbors，不发送 Interest 包）。
- 拓扑通过 `infra/topology/*` 配置，支持按 `NETOS_TOPOLOGY_RELOAD_MS` 周期热加载（无协议消息）。

## 4) 最常用入口（运行 & 演示）
**推荐一条命令：**
```
COMPOSE_BAKE=true ./scripts/demo.sh table-stats --with-health
```
**常用命令：**
```
./scripts/demo.sh validate
./scripts/demo.sh trace
./scripts/demo.sh table-stats --with-health
./scripts/demo.sh hop-story --3-node
```

## 5) 模块地图（简版）
- `src/netos/core/`：配置/日志/消息编解码/字符串工具
- `src/netos/cache/`：Redis Unix socket 客户端
- `src/netos/network/`：UDP 传输 + send_direct/broadcast + 拓扑
- `src/netos/tables/`：QueryTable / SyncTable / Bloom Filter 表
- `src/netos/node/`：请求/数据管线 + Node 主循环
- `infra/`：Dockerfile/entrypoint/compose/topology
- `scripts/`：demo 驱动、验证、观测、故障注入
- `docs/`：Quickstart/Runbook/Observability/Contracts

## 6) 行为契约（Phase‑1）
- `req_state=drop_invalid|drop_ttl|drop_duplicate|serve_local|forward`
- `data_state=drop_invalid|drop_not_origin|store_local`
- 详见：`docs/CONTRACTS.md`

## 7) 可观测性入口
- `docs/OBSERVABILITY.md`（字段定义）
- `scripts/trace_demo.sh` / `table_stats_demo.sh` / `inspect_demo.sh`

## 8) 已知缺口 / 下一阶段候选
- 订阅式 Push 未做（Push 阶段仅为 CBF 交换）
- QueryTable 语义仍为“单源 request_id 去重”
- 动态拓扑发现与规模测试未做

## 9) 下一步建议（可选）
1. 继续完善 BF TTL/aging 细节与调参策略
2. 定义 `request_id` 合并策略
3. 事件驱动 + 多线程队列化（替换同步 for‑loop）
4. 评测脚本与 metrics

## 10) 断档恢复流程（执行顺序）
1. 先读本文件：`docs/CONTINUITY.md`
2. 再读：`docs/AGENT_CONTEXT.md`、`docs/PROTOTYPE_OVERVIEW.md`
3. 运行推荐命令验证 demo
4. 开始下一阶段前，更新本文件「更新记录」

## 11) 更新记录
- 2026‑03‑13：建立此文档，作为断档恢复入口。
- 2026‑03‑13：基于会议记录重排 Phase 2.3 / Phase 3（传播控制 + 聚合前置，Bloom/Push/异步后置）。
- 2026‑03‑15：完成 Phase 3.1 Bloom Filter 优化（Query-BF 分裂阈值 + 填充率指标）。
- 2026‑03‑16：澄清 Push 阶段为 CBF 周期摘要交换（不发送 Interest 包）。
- 2026‑03‑16：完成 Phase 3.3 异步转发（转发队列 + worker 模型 + drop_newest）。
- 2026‑03‑16：完成 Phase 3.4 动态拓扑热加载（文件配置重载）。
- 2026‑03‑16：加入 NS-3 scaffold（文档 + 拓扑导出脚本）。
