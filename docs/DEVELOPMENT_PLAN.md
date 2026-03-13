# NetOS Prototype Development Plan (Phase 1→2)

This plan consolidates the end-to-end roadmap with **current progress**. It is intentionally practical and demo-driven.

Legend:
- ✅ Done
- 🟡 In progress
- ⬜ Not started

---

## Phase 0 - Bootstrap & Scaffold

- ✅ Repo initialized, C++20 skeleton, CMake build
- ✅ Dockerfile + docker-compose scaffolding
- ✅ Basic README & doc structure

## Phase 1 - Minimal Pull Path Demo (Current Focus)

### 1.1 Core runnable demo
- ✅ 2-node demo (REQ/DATA exchange, Redis socket, UDP control plane)
- ✅ 3-node hop-through demo (node1 → node2 → node3)
- ✅ Containerized run path via docker compose

### 1.2 Behavior correctness & observability
- ✅ Structured req/data state logs (`req_state=`, `data_state=`)
- ✅ QueryTable duplicate suppression (TTL)
- ✅ SyncTable LRU stub for origin tracking
- ✅ Edge-case helper (drop_invalid, drop_ttl, drop_not_origin)
- ✅ Table stats (QueryTable/SyncTable stats logged)
- ✅ Table stats summary helper (`table-stats`)
- ✅ Table stats health summary helper (`table_stats_health.sh`)

### 1.3 Demo narration & validation
- ✅ validate/trace/inspect helpers
- ✅ Pull path summary helper
- ✅ 3-node hop story + story beats
- ✅ Unified demo driver (`scripts/demo.sh`)
- ✅ Quickstart / Runbook / Handoff / Demo Map / Overview / Contracts / Observability docs

### Recommended Demo Path
- Recommended one-liner (table stats + health): `COMPOSE_BAKE=true ./scripts/demo.sh table-stats --with-health` (starts via docker compose; Bake delegation enabled).
- Staged flow sequence: see `docs/QUICKSTART.md` → "Demo Flow (Staged Presentation)" (validate → trace → table-stats → table-health → hop-story → pull-path → inspect).

### 1.4 Testing
- ✅ Minimal CTest smoke test for core parsing/config

---

## Phase 1.5 - Consolidation & Developer Clarity (Short Term)

- ✅ Request/Data pipelines refactored into explicit modules
- ✅ Pipeline order documented in Observability
- ✅ Make table-stats output more human-readable (summary line)
- ✅ Demo driver, quickstart, handoff, demo map aligned on the recommended one-liner

---

## Phase 2 - Structured Evolution (Planned, Not Started)

### 2.1 Protocol & table semantics
- ⬜ Formalize network API as `send_direct` + `send_broadcast`, topology via config
- ⬜ Enforce QueryTable single-source entries (request_id only; duplicates dropped regardless of origin)
- ⬜ Keep QueryTable distinct from Query-BF aggregation buffers (QueryTable is request-id dedupe only)
- ⬜ SyncTable stores file name (not Content-BF), LRU eviction by key
- ⬜ Remove SYNC packet; DATA updates SyncTable state (meeting decision: no standalone SYNC packet).

### 2.2 Enhanced demo scenarios
- ⬜ Repeat request / dedupe demo scenario
- ✅ Failure injection (timeouts, drop paths) demo
- ✅ Kathara-based 3-node linear demo (initial small topology)
- ✅ Kathara-based topology tests (4-node hub)
- ⬜ Kathara-based topology tests (larger mesh)

### 2.3 Propagation Control & Aggregation (Meeting-Driven)
- ⬜ Introduce two BF types: Content-BF (neighbor cache summaries) and Query-BF (broadcast query aggregation)
- ⬜ BF spec configurable (size, hashes, exchange interval, TTL/aging)
- ⬜ Local miss flow: check neighbor Content-BF for a direct query; if no hit, aggregate keys into per-origin Query-BF and broadcast
- ⬜ Query-BF aggregation is per-origin only (no cross-origin merges)
- ⬜ Query-BF handling: local match → respond; otherwise forward with ttl-1
- ⬜ Broadcast suppression policy (TTL/aging/attempt limits) for Query-BF forwarding
- ⬜ Lightweight metrics for drop/duplicate/broadcast counts

---

## Phase 3 - Advanced Mechanisms (Future)

### 3.1 Bloom Filter Optimization
- ⬜ Content-BF exchange tuning (intervals, size, hash count)
- ⬜ Query-BF split/merge strategy (control false positives)
- ⬜ BF TTL/aging refinements
- ⬜ Prefix/level aggregation for key names

### 3.2 Push Stage / Subscription
- ⬜ Push pipeline / subscription logic (Kafka-like model)
- ⬜ Push is not constrained to reverse path
- ⬜ SyncTable-driven destinations

### 3.3 Async Forwarding / Buffering
- ⬜ Receive/forward queue separation
- ⬜ Worker loop model (line-card style)

### 3.4 Dynamic Topology & Evaluation
- ⬜ Dynamic topology discovery
- ⬜ NS-3 evaluation path (as referenced in meetings)

---

## Current Status Summary

- Phase 1 complete and stable, with rich demo/validation tooling.
- Phase 1.5 complete (output polish + doc alignment done).
- Phase 2+ items remain intentionally deferred.
