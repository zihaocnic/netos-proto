# NetOS Prototype Status (Continuity Log)

Purpose: Keep a stable, always‑up‑to‑date snapshot so session resets don’t lose context.
Repo: `/home/ubuntu/Project/NetOS/netos-proto`

## Snapshot (Current State)
- Phase 1 demo is **complete and runnable** (Pull path only).
- 2‑node demo: node B requests a key, node A serves it, node B stores locally.
- 3‑node demo: hop‑through pull across linear topology (compose + Kathara).
- Demo driver + helpers exist (validate/trace/hop-story/table-stats/inspect).
- Kathara 3-node + 4-node hub demo scaffolding added (see `docs/KATHARA_DEMO.md`).
- Repo initialized; Git history in place (per prior notes).

## Guardrails (Phase 1)
- Keep `req_state=` / `data_state=` log labels stable (scripts depend on them).
- No Push pipeline / Bloom filters in Phase 1.
- Static topology via env files under `infra/topology/`.

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
