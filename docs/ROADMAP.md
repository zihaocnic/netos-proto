# NetOS Demo Roadmap

This roadmap focuses on the runnable demo prototype in this repository. It mirrors the upstream research milestones but keeps scope small and incremental.

## Milestones

1. **M0 — Documentation & Scaffold (Done)**
- Document tech stack constraints and architecture direction
- Create C++20 project skeleton and container layout

2. **M1 — Minimal 2-Node Demo (Done)**
- Redis + sync app in the same container
- UDP request/data exchange
- QueryTable + SyncTable skeletons wired

3. **M2 — Loop Control & TTL (Next)**
- Request TTL handling improvements
- Secondary-BF style suppression stub
- Structured metrics/logging for drop reasons

4. **M3 — Buffer & Async Forwarding (Planned)**
- Separate receive/forward queues
- Basic worker loop to simulate line-card behavior

5. **M4 — Bloom Filter Exchange (Planned)**
- Local BF maintenance
- Periodic BF broadcast to neighbors
- Prefix-match stub for key aggregation

6. **M5 — Kathara/NS-3 Ready (Planned)**
- Config-file topology parser
- Docker/Kathara topology configs
- Demo at 4-node scale

## Notes
- The upstream architecture calls for a full Pull + Push pipeline and deeper topology handling. The demo roadmap intentionally stops before implementing full Push.
