# NetOS Prototype Development Plan (Phase 1→2)

This plan consolidates the end‑to‑end roadmap with **current progress**. It is intentionally practical and demo‑driven.

Legend:
- ✅ Done
- 🟡 In progress
- ⬜ Not started

---

## Phase 0 — Bootstrap & Scaffold

- ✅ Repo initialized, C++20 skeleton, CMake build
- ✅ Dockerfile + docker-compose scaffolding
- ✅ Basic README & doc structure

## Phase 1 — Minimal Pull Path Demo (Current Focus)

### 1.1 Core runnable demo
- ✅ 2‑node demo (REQ/DATA exchange, Redis socket, UDP control plane)
- ✅ 3‑node hop‑through demo (node1 → node2 → node3)
- ✅ Containerized run path via docker compose

### 1.2 Behavior correctness & observability
- ✅ Structured req/data state logs (`req_state=`, `data_state=`)
- ✅ QueryTable duplicate suppression (TTL)
- ✅ SyncTable LRU stub for origin tracking
- ✅ Edge‑case helper (drop_invalid, drop_ttl, drop_not_origin)
- ✅ Table stats (QueryTable/SyncTable stats logged)
- ✅ Table stats summary helper (`table-stats`)
- ✅ Table stats health summary helper (`table_stats_health.sh`)

### 1.3 Demo narration & validation
- ✅ validate/trace/inspect helpers
- ✅ Pull path summary helper
- ✅ 3‑node hop story + story beats
- ✅ Unified demo driver (`scripts/demo.sh`)
- ✅ Quickstart / Runbook / Handoff / Demo Map / Overview / Contracts / Observability docs

### Recommended Demo Path
- Recommended one-liner (table stats + health): `COMPOSE_BAKE=true ./scripts/demo.sh table-stats --with-health` (starts via docker compose; Bake delegation enabled).
- Staged flow sequence: see `docs/QUICKSTART.md` → "Demo Flow (Staged Presentation)" (validate → trace → table-stats → table-health → hop-story → pull-path → inspect).

### 1.4 Testing
- ✅ Minimal CTest smoke test for core parsing/config

---

## Phase 1.5 — Consolidation & Developer Clarity (Short Term)

- ✅ Request/Data pipelines refactored into explicit modules
- ✅ Pipeline order documented in Observability
- ✅ Make table‑stats output more human‑readable (summary line)
- ✅ Demo driver, quickstart, handoff, demo map aligned on the recommended one-liner

---

## Phase 2 — Structured Evolution (Planned, Not Started)

### 2.1 Protocol & table semantics
- ⬜ Formalize network API as `send_direct` + `send_broadcast`, topology via config
- ⬜ Enforce QueryTable single‑source entries (no multi‑requester coalescing)
- ⬜ SyncTable stores file name (not BF), LRU eviction policy
- ⬜ Remove SYNC packet; DATA updates SyncTable state

### 2.2 Enhanced demo scenarios
- ⬜ Repeat request / dedupe demo scenario
- ⬜ Failure injection (timeouts, drop paths) demo
- ⬜ Kathara‑based topology tests (4‑node / hub → larger mesh)

### 2.3 Optional performance instrumentation (lightweight)
- ⬜ Simple counters/timers (not full metrics system)

---

## Phase 3 — Advanced Mechanisms (Future)

- ⬜ Bloom Filter exchange (unified length)
- ⬜ Periodic BF sync with batching/chunking across neighbors
- ⬜ Push pipeline / subscription logic
- ⬜ Async forwarding / buffer separation
- ⬜ Dynamic topology discovery

---

## Current Status Summary

- Phase 1 complete and stable, with rich demo/validation tooling.
- Phase 1.5 in progress (minor output polish + doc alignment).
- Phase 2+ items remain intentionally deferred.
