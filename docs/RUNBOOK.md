# NetOS Demo Runbook

This runbook collects the most common operator and developer actions for the Phase-1 demo prototype.
For a staged demo flow, see `docs/QUICKSTART.md`. For a compact demo index, see `docs/DEMO_MAP.md`.
For onboarding, see `docs/HANDOFF.md`.

## Prerequisites

- Docker with docker compose v2.
- Optional: CMake + a C++20 compiler for local builds and tests.

## Primary Entry Points

Unified demo driver (recommended):
```bash
./scripts/demo.sh start
./scripts/demo.sh validate
./scripts/demo.sh trace
```

2-node demo (default):
```bash
docker compose -f infra/docker-compose.yml up --build
```

3-node demo (hop-through):
```bash
docker compose -f infra/docker-compose.3-node.yml up --build
```

Validation helpers:
```bash
./scripts/validate_demo.sh
./scripts/validate_3node_demo.sh
```

Unified validation:
```bash
./scripts/demo.sh validate
./scripts/demo.sh validate --3-node
```

Narration helpers:
```bash
./scripts/trace_demo.sh
./scripts/hop_story_3node.sh
COMPOSE_FILE=infra/docker-compose.3-node.yml ./scripts/pull_path_demo.sh
```

Unified narration:
```bash
./scripts/demo.sh trace
./scripts/demo.sh pull-path --3-node
./scripts/demo.sh hop-story --3-node
```

Inspection helper:
```bash
./scripts/inspect_demo.sh
```

Topology sanity check:
```bash
./scripts/check_topology_env.sh
```

## Expected Log Signals

- Request states: `req_state=originated|forward|serve_local|drop_*`
- Data states: `data_state=store_local|drop_*`

Use this pattern to scan the logs quickly:
```bash
docker compose -f infra/docker-compose.yml logs --no-color | grep -E "req_state=|data_state="
```

## Local Build And Tests (Optional)

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Troubleshooting

- No `data_state=store_local` observed:
  - Confirm docker compose is running and containers are healthy.
  - Run `./scripts/check_topology_env.sh` to validate topology env files.
  - Inspect logs with the grep command above.

- Demo stops or exits early:
  - Ensure Docker is running and ports are available.
  - Run `./scripts/validate_demo.sh` for a self-contained check.

## Clean Shutdown

```bash
docker compose -f infra/docker-compose.yml down -v
```

Use the 3-node compose file instead if that demo is running.
