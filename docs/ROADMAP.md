# NetOS Demo Roadmap

This roadmap focuses on the runnable demo prototype in this repository. It mirrors the upstream research milestones but keeps scope small and incremental.

## Phase 1 Scope

Phase 1 corresponds to the minimal Pull loop from the meeting-note architecture (current M1). It keeps QueryTable + SyncTable stubs, UDP request/data messaging, and a static topology while deferring Content-BF/Query-BF workflows, Push, and async forwarding.

## Milestones

1. **M0 — Documentation & Scaffold (Done)**
- Document tech stack constraints and architecture direction
- Create C++20 project skeleton and container layout

2. **M1 — Minimal 2-Node Demo (Done)**
- Redis + sync app in the same container
- UDP request/data exchange
- QueryTable + SyncTable skeletons wired

3. **M2 — Propagation Control & Aggregation (Next)**
- Introduce two BF types: Content-BF (neighbor cache summaries) and Query-BF (broadcast query aggregation)
- Local miss flow: Content-BF direct query; else per-origin Query-BF aggregation + broadcast
- Query-BF aggregation per-origin only (no cross-origin merges)
- Query-BF handling: local match → respond; otherwise forward with ttl-1
- Broadcast suppression policy (TTL/aging/attempt limits) for Query-BF forwarding
- Structured metrics/logging for drop reasons

4. **M3 — Buffer & Async Forwarding (Planned)**
- Separate receive/forward queues
- Basic worker loop to simulate line-card behavior

5. **M4 — Bloom Filter Optimization (Planned)**
- Content-BF exchange tuning (interval, size, hash count, TTL/aging)
- Query-BF split/merge strategy (false-positive control)
- Prefix/level aggregation for key names

6. **M5 — Dynamic Topology & Evaluation (Planned)**
- Dynamic topology discovery
- NS-3 evaluation path
- Kathara/NS-3 topology configs

## Notes
- The upstream architecture calls for a full Pull + Push pipeline and deeper topology handling. The demo roadmap intentionally stops before implementing full subscription-based Push.
- The prototype’s Push stage is defined as periodic Content-BF (CBF) summary exchange (local cache -> neighbors), with no Interest packets.
