# NetOS Prototype Handoff

This guide is a concise handoff for a new developer or a meeting host.

## What This Repo Is
- Phase-1 NetOS sync prototype focused on the Pull path.
- Redis + netos-node per container, UDP request/data exchange.
- Demo-first artifact with validation and narration helpers.

## First 10 Minutes
1. Read `docs/QUICKSTART.md`.
2. Run `./scripts/demo.sh validate`.
3. Run `./scripts/demo.sh trace`.
4. Run `./scripts/demo.sh hop-story --3-node`.

## Where To Look
- `docs/PROTOTYPE_OVERVIEW.md` for the architecture summary.
- `docs/RUNBOOK.md` for operator commands.
- `docs/CONTRACTS.md` for behavior guarantees.
- `docs/OBSERVABILITY.md` for log fields.
- `design/ARCHITECTURE.md` for the architecture snapshot.

## Repo Map
- `src/` C++20 node, query table, sync table, network, cache.
- `infra/` docker assets, compose files, topology envs.
- `scripts/` demo driver and validation helpers.
- `docs/` overview, runbook, contracts, observability.

## Guardrails For Phase 1
- Keep `req_state=` and `data_state=` labels stable for scripts.
- Preserve the demo driver interface in `scripts/demo.sh`.
- No Push pipeline or Bloom filters in Phase 1.

## Common Extensions
- Add a topology directory under `infra/topology/` and run `./scripts/check_topology_env.sh`.
- Add a demo key using `NETOS_SEED_KEYS` and `NETOS_REQUEST_KEYS` in the topology env files.
- Update `docs/CONTRACTS.md` and `docs/OBSERVABILITY.md` when behavior changes.

## Validation Checklist
- `./scripts/check_topology_env.sh`
- `./scripts/demo.sh validate`
- `./scripts/demo.sh validate --3-node`
- `ctest --test-dir build --output-on-failure` after a local build
