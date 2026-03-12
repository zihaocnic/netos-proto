# NetOS Demo Quickstart

This is the fastest path to run and present the Phase-1 NetOS demo.
For a compact index of demo flows and helpers, see `docs/DEMO_MAP.md`.

## Prerequisites
- Docker with docker compose v2.
- Optional: CMake + a C++20 compiler for local tests.

## Quickstart (2-Node, One-Shot)
1. Run `./scripts/demo.sh trace`.
2. Run `./scripts/demo.sh table-stats --with-health` to capture the QueryTable/SyncTable snapshot plus the health summary.
3. Optional: run `./scripts/demo.sh table-health` for a standalone health summary derived from table stats.
4. Optional: run `./scripts/demo.sh validate` for a pass/fail check.

One-liner (table stats + health): `./scripts/demo.sh table-stats --with-health`.

## Demo Flow (Staged Presentation)
1. Sanity check (2 minutes). Command: `./scripts/demo.sh validate`.
2. Pull-path narration (5 minutes). Command: `./scripts/demo.sh trace`.
3. Table stats snapshot + health (7 minutes). Command: `./scripts/demo.sh table-stats --with-health`.
4. Optional table stats health check (7.5 minutes). Command: `./scripts/demo.sh table-health`.
5. Hop-through story (8 minutes). Command: `./scripts/demo.sh hop-story --3-node`.
6. Pull-path summary (10 minutes). Command: `./scripts/demo.sh pull-path --3-node`.
7. Optional Redis inspection (12 minutes). Command: `./scripts/demo.sh inspect --3-node`.

## One-Shot Helpers
These commands start the demo, wait for the expected key, print output, and stop the stack.
- `./scripts/demo.sh validate`
- `./scripts/demo.sh trace`
- `./scripts/demo.sh table-stats`
- `./scripts/demo.sh table-stats --with-health`
- `./scripts/demo.sh table-health`
- `./scripts/demo.sh pull-path`
- `./scripts/demo.sh hop-story --3-node`
- `./scripts/demo.sh inspect`
- `./scripts/demo.sh edge-cases`

## Manual Mode (Keep The Stack Running)
1. Start in the foreground. Command: `./scripts/demo.sh start`.
2. Start in the background. Command: `./scripts/demo.sh start --detach`.
3. Follow logs. Command: `docker compose -f infra/docker-compose.yml -p netos-demo logs -f`.
4. Stop and clean. Command: `docker compose -f infra/docker-compose.yml -p netos-demo down -v`.

Defaults: `netos-demo` is the 2-node compose project name. `netos-demo-3node` is the 3-node project name. Use `--3-node` with the demo driver or set `COMPOSE_FILE` to the 3-node compose file.

## Overrides
- `EXPECT_KEY=beta` to change the expected key.
- `TIMEOUT_SEC=40` to extend helper timeouts.
- `COMPOSE_FILE=...` to point at a custom compose file.
- `COMPOSE_PROJECT_NAME=...` to avoid collisions.

## Next Reads
- `docs/PROTOTYPE_OVERVIEW.md`
- `docs/RUNBOOK.md`
- `docs/OBSERVABILITY.md`
- `docs/DEMO_MAP.md`
