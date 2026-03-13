# NetOS Prototype Overview (Phase 1 + Phase 2.3)

This repository is a runnable prototype of the NetOS synchronization layer. It is intentionally small and optimized for demo clarity, not production completeness.

## What This Prototype Is

- A minimal, runnable Pull-path demo aligned to the meeting-note architecture.
- A Redis-proxy model where Redis and the sync app co-reside in one container.
- A UDP request/data exchange that proves the core sync loop.
- A documentation-heavy artifact that is easy to explain and verify.

## What This Prototype Is Not

- No Push pipeline or subscription-based replication.
- No async forwarding, dynamic topology discovery, or scale testing.

## Key Capabilities (Today)

- 2-node demo: node B requests a key, node A serves it, node B stores it.
- 3-node demo: hop-through pull across a linear topology (node1 -> node2 -> node3).
- Duplicate suppression via QueryTable TTL.
- SyncTable LRU stub for tracking served origins.
- Content-BF neighbor cache summaries and Query-BF broadcast aggregation (Phase 2.3).
- Structured request/data state logs for narration and validation.

## Fast Demo Narrative (5 Minutes)

1. Run the 2-node demo and point to `req_state=` and `data_state=` logs.
2. Validate the demo with `./scripts/validate_demo.sh`.
3. Run the 3-node hop-through demo and use `./scripts/hop_story_3node.sh` to show the hop chain.
4. Use `COMPOSE_FILE=infra/docker-compose.3-node.yml ./scripts/pull_path_demo.sh` to summarize the pull path.

## Where To Look

- Quickstart and staged demo flow: `docs/QUICKSTART.md`
- Demo paths and helper index: `docs/DEMO_MAP.md`
- Architecture snapshot: `design/ARCHITECTURE.md`
- Behavior contracts: `docs/CONTRACTS.md`
- Observability and log fields: `docs/OBSERVABILITY.md`
- 3-node walkthrough: `docs/DEMO_3_NODE.md`
- Runbook entry points: `docs/RUNBOOK.md`
- Handoff guide: `docs/HANDOFF.md`

## Current Boundaries

Phase 1 + Phase 2.3 stop after the minimal Pull loop plus propagation control. This keeps the prototype reportable
and demo-ready while leaving space for later phases.
