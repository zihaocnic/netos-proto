# NetOS Demo Roadmap

This roadmap focuses on the runnable demo prototype in this repository. It mirrors the upstream research milestones but keeps scope small and incremental.

## Phase 1 Scope

Phase 1 corresponds to the minimal Pull loop from the meeting-note architecture (current M1). It keeps QueryTable + SyncTable stubs, UDP request/data messaging, and a static topology while deferring Push, Bloom filters, and async forwarding.

## Milestones

1. **M0 — Documentation & Scaffold (Done)**
- Document tech stack constraints and architecture direction
- Create C++20 project skeleton and container layout

2. **M1 — Minimal 2-Node Demo (Done)**
- Redis + sync app in the same container
- UDP request/data exchange
- QueryTable + SyncTable skeletons wired

3. **M2 — Propagation Control & Aggregation (Next)**
- Request TTL handling improvements
- Unique request ID strategy
- Broadcast suppression policy (TTL/aging/limits)
- Request aggregation (merge same-key queries)
- Structured metrics/logging for drop reasons

4. **M3 — Buffer & Async Forwarding (Planned)**
- Separate receive/forward queues
- Basic worker loop to simulate line-card behavior

5. **M4 — Bloom Filter Exchange (Planned)**
- Local BF maintenance
- Periodic BF broadcast to neighbors
- BF merge threshold + split strategy
- Prefix/level aggregation for key names

6. **M5 — Dynamic Topology & Evaluation (Planned)**
- Dynamic topology discovery
- NS-3 evaluation path
- Kathara/NS-3 topology configs

## Notes
- The upstream architecture calls for a full Pull + Push pipeline and deeper topology handling. The demo roadmap intentionally stops before implementing full Push.
