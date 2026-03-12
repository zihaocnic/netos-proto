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

### 1.3 Demo narration & validation
- ✅ validate/trace/inspect helpers
- ✅ Pull path summary helper
- ✅ 3‑node hop story + story beats
- ✅ Unified demo driver (`scripts/demo.sh`)
- ✅ Quickstart / Runbook / Handoff / Demo Map / Overview / Contracts / Observability docs

### 1.4 Testing
- ✅ Minimal CTest smoke test for core parsing/config

---

## Phase 1.5 — Consolidation & Developer Clarity (Short Term)

- ✅ Request/Data pipelines refactored into explicit modules
- ✅ Pipeline order documented in Observability
- 🟡 Make table‑stats output more human‑readable (summary line)
- 🟡 Minor doc polish to keep entry points aligned

---

## Phase 2 — Structured Evolution (Planned, Not Started)

### 2.1 Clearer internal abstractions
- ⬜ Formalize Request/Data pipeline interfaces for extension
- ⬜ Add explicit hook points for future BF/Push components (no functionality yet)

### 2.2 Enhanced demo scenarios
- ⬜ Repeat request / dedupe demo scenario
- ⬜ Failure injection (timeouts, drop paths) demo
- ⬜ Larger topology (4‑node / hub) demo

### 2.3 Optional performance instrumentation (lightweight)
- ⬜ Simple counters/timers (not full metrics system)

---

## Phase 3 — Advanced Mechanisms (Future)

- ⬜ Bloom Filter exchange
- ⬜ Push pipeline / subscription logic
- ⬜ Async forwarding / buffer separation
- ⬜ Dynamic topology discovery

---

## Current Status Summary

- Phase 1 complete and stable, with rich demo/validation tooling.
- Phase 1.5 in progress (minor output polish + doc alignment).
- Phase 2+ items remain intentionally deferred.
