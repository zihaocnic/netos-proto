# NetOS Prototype Status (Continuity Log)

Purpose: Keep a stable, always‑up‑to‑date snapshot so session resets don’t lose context.
Repo: `/home/ubuntu/Project/NetOS/netos-proto`

## Snapshot (Current State)
- Phase 1 demo is **complete and runnable**, plus Phase 2.3 propagation control (Content-BF + Query-BF) and Phase 3.3 async forwarding.
- Content-BF direct hits use a fallback window (`NETOS_CONTENT_BF_FALLBACK_MS`) before scheduling Query-BF broadcast.
- Query-BF handling responds to all local matches and still forwards (ttl-1 + attempt limits).
- Query-BF aggregation now splits on fill-ratio / max-keys thresholds and logs fill ratios.
- Async forwarding queue separates receive/forward, default-enabled with drop-newest overflow policy.
- Dynamic topology reload via periodic env-file re-read (`NETOS_TOPOLOGY_RELOAD_MS`).
- 2‑node demo: node B requests a key, node A serves it, node B stores locally.
- 3‑node demo: hop‑through pull across linear topology (compose + Kathara).
- Demo driver + helpers exist (validate/trace/hop-story/table-stats/inspect).
- Kathara 3-node + 4-node hub demo scaffolding added (see `docs/KATHARA_DEMO.md`).
- Repo initialized; Git history in place (per prior notes).

## Guardrails (Phase 1)
- Keep `req_state=` / `data_state=` log labels stable (scripts depend on them).
- No subscription-based Push pipeline; Push stage is the periodic Content-BF (CBF) summary exchange (local cache -> neighbors) with no Interest packets.
- Content-BF/Query-BF Bloom filters are now part of Phase 2.3.
- Topology via env files under `infra/topology/`, with optional periodic reload (`NETOS_TOPOLOGY_RELOAD_MS`).

## Key Decisions (So Far)
- Language: C++20.
- Redis + sync app co‑located in each container (unix socket by default).
- UDP control plane with `REQ` / `DATA` messages.
- QueryTable TTL for duplicate suppression.
- SyncTable LRU stub to track served origins.
- Demo‑first, documentation‑heavy, architecture‑aligned but intentionally minimal.

## Current Demo Commands (Fast Path)
- `./scripts/demo.sh validate`
- `./scripts/demo.sh trace`
- `./scripts/demo.sh table-stats --with-health`
- `./scripts/demo.sh hop-story --3-node`

## Next Up (Phase 2 Candidates)
- Formalize network API (`send_direct` + `send_broadcast`).
- QueryTable single‑source semantics (drop duplicates regardless of origin).
- SyncTable store file name; remove standalone SYNC packet (DATA updates state).
- Repeat‑request / dedupe demo scenario.
- Failure‑injection demo (timeouts, drop paths).
- Kathara‑style topology tests (4‑node+).
- Lightweight counters/timers (not full metrics system).

## Open Questions
- Which Phase‑2 item should we tackle first?
- Any changes to demo topology or key scenarios?
- Confirm GitHub remote/branch workflow for future pushes.

## Log
- 2026‑03‑13: Session reset; reconstructed context from repo docs; created `STATUS.md` for continuity.
- 2026‑03‑15: Added Phase 3.1 Bloom Filter optimization (split thresholds + fill-ratio metrics).
- 2026‑03‑16: Implemented Phase 3.3 async forwarding (queue + worker loop + drop-newest).
- 2026‑03‑16: Implemented Phase 3.4 dynamic topology reload (file-based).
