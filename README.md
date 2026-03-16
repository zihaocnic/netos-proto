# NetOS Demo Prototype

Minimal runnable demo of the NetOS synchronization layer based on meeting notes. It keeps the Redis-proxy model and co-locates Redis + sync app in the same container, using a small UDP request/data exchange to validate the architecture direction.

Start here for a concise summary and demo entry points:
- `docs/QUICKSTART.md`
- `docs/DEMO_MAP.md`
- `docs/PROTOTYPE_OVERVIEW.md`
- `docs/RUNBOOK.md`
- `docs/HANDOFF.md`

## Quick Start (Docker Compose)

```bash
docker compose -f infra/docker-compose.yml up --build
```

The default two-node topology is defined in:
- `infra/topology/2-node/node1.env`
- `infra/topology/2-node/node2.env`

You should see logs like:
- Node A seeds key `alpha`
- Node B broadcasts a request for `alpha`
- Node A serves `alpha`
- Node B stores `alpha` locally

Stop with `Ctrl+C`.

## Unified Demo Driver (Recommended)

Use the single entry-point script to start, validate, and narrate demos:

```bash
./scripts/demo.sh start
./scripts/demo.sh validate
./scripts/demo.sh trace
```

For the 3-node hop-through demo:

```bash
./scripts/demo.sh start --3-node
./scripts/demo.sh validate --3-node
./scripts/demo.sh hop-story --3-node
```

For a staged demo flow and one-shot helpers, see `docs/QUICKSTART.md`.

## 3-Node Demo (Linear Hop-Through)

```bash
docker compose -f infra/docker-compose.3-node.yml up --build
```

This demo seeds `alpha` on node1. Node2 requests it first and caches it, then
node3 requests it and gets the key via node2. This shows a hop-through pull
across a simple linear topology.

```bash
./scripts/validate_3node_demo.sh
```

For a step-by-step hop-through story (timeline + hop chain), run:

```bash
./scripts/hop_story_3node.sh
```

See `docs/DEMO_3_NODE.md` for details on the expected log events.

## Phase 1 Scope (Meeting-Note Mapping)

Phase 1 targets the minimal **Pull** loop described in the meeting notes. It keeps module boundaries aligned with the long-term architecture while staying intentionally small.

- Node + local cache: implemented (`netos-node` + Redis unix socket).
- QueryTable duplicate suppression: implemented (TTL map).
- SyncTable destination tracking: implemented as a small LRU stub.
- Control-plane messaging: UDP `REQ`/`DATA` only.
- Topology management: neighbor list via per-node env files, with optional periodic reload (`NETOS_TOPOLOGY_RELOAD_MS`).
- Push pipeline: out of scope for Phase 1 (CBF summary exchange only).

Phase 2.3 adds Content-BF/Query-BF workflows, and Phase 3.3 adds async forwarding.

## Behavior Contracts (Phase 1)

For explicit guarantees and non-guarantees in the current prototype, see
`docs/CONTRACTS.md`.

## Table Semantics (Demo)

- **QueryTable**: TTL map of **request IDs** used to suppress duplicate/looping requests.
- **SyncTable**: LRU map of **key → origin node IDs** for requests this node served (stub for future push).

## Local Build (Optional)

```bash
cmake -S . -B build
cmake --build build -j
./build/netos-node
```

## Core Tests (Lightweight)

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

This runs `netos-core-tests` (message parsing, string utils, config validation) and does not
require Docker or Redis.

## Validation (Docker Compose)

```bash
./scripts/validate_demo.sh
```

This script brings the demo up, waits for the `data_state=store_local` log line for `alpha`, and tears the stack down.

For the 3-node hop-through demo, use:

```bash
./scripts/validate_3node_demo.sh
```

## Topology Sanity Check

```bash
./scripts/check_topology_env.sh
```

This script validates the topology env files for required fields and unique
node IDs/ports per topology directory.

## NS-3 Scaffold

See `docs/NS3.md` for the NetOS-to-NS-3 mapping notes and the lightweight
topology export. The export script lives at `scripts/ns3/generate_topology.py`.

## Trace Helper

```bash
./scripts/trace_demo.sh
```

This helper runs the demo and prints the request/data state trace lines plus a compact
timeline view for the expected key.

## Table Stats Summary Helper

```bash
./scripts/table_stats_demo.sh
```

This helper runs the demo and prints the latest QueryTable and SyncTable stats per node
based on the `query_table_*` and `sync_table_*` log fields.

## Pull Path Summary Helper

```bash
./scripts/pull_path_demo.sh
COMPOSE_FILE=infra/docker-compose.3-node.yml ./scripts/pull_path_demo.sh
```

This helper runs the demo, waits for the expected key, and prints a compact per-request
summary showing `origin -> served_by -> stored_at` for the pull path, plus a short
pull-story narrative per request ID. Node labels reflect the docker compose service names
(`node1`, `node2`, `node3` in the default topologies).

## Hop-Through Story Helper

```bash
./scripts/hop_story_3node.sh
```

This helper runs the 3-node demo and prints a hop-through timeline, story beats, and
a compact hop chain summary for the expected key.

## Inspection Helper

```bash
./scripts/inspect_demo.sh
```

This helper runs the demo, waits for the expected key to land, and prints per-node Redis
state (db size, keys, and selected key values) plus the config summary line.
Use `INSPECT_KEYS=alpha,beta` to check multiple keys in one run.

## Edge-Case Helper

```bash
./scripts/observe_edge_cases.sh
```

This helper starts the 2-node demo, injects a few crafted UDP messages, and confirms:
- `req_state=drop_invalid` for malformed requests
- `req_state=drop_ttl` for expired requests
- `data_state=drop_not_origin` for data sent to the wrong node

## Observability

See `docs/OBSERVABILITY.md` for log field definitions, request/data state labels,
and quick grep examples.

## Environment Variables

- `NETOS_NODE_ID`: node name (e.g., `node1`)
- `NETOS_BIND_IP`: UDP bind IP (default `0.0.0.0`)
- `NETOS_BIND_PORT`: UDP port (default `9000`)
- `NETOS_NEIGHBORS`: comma list of `host:port` entries
- `NETOS_CONFIG_FILE`: path to a node config env file (loaded before process env)
- `NETOS_TOPOLOGY_DIR`: directory holding `NODE_ID.env` files (requires `NETOS_NODE_ID`)
- `NETOS_REDIS_SOCKET`: Unix socket for Redis (default `/var/run/redis/redis.sock`)
- `NETOS_SEED_KEYS`: semicolon list `k=v;k2=v2`
- `NETOS_REQUEST_KEYS`: comma list of keys to request at startup
- `NETOS_REQUEST_DELAY_MS`: delay before requesting keys
- `NETOS_REQUEST_TTL`: request TTL (default `3`)
- `NETOS_QUERY_TTL_MS`: QueryTable TTL (default `1500`)
- `NETOS_SYNC_TABLE_CAPACITY`: SyncTable capacity (default `128`)
- `NETOS_CONTENT_BF_BITS`: Content-BF size in bits (default `512`)
- `NETOS_CONTENT_BF_HASHES`: Content-BF hash count (default `3`)
- `NETOS_CONTENT_BF_EXCHANGE_MS`: Content-BF exchange interval (default `1000`)
- `NETOS_CONTENT_BF_TTL_MS`: Content-BF aging/TTL window (default `3000`)
- `NETOS_CONTENT_BF_FALLBACK_MS`: Content-BF direct miss fallback window (default `300`)
- `NETOS_QUERY_BF_BITS`: Query-BF size in bits (default `1024`)
- `NETOS_QUERY_BF_HASHES`: Query-BF hash count (default `4`)
- `NETOS_QUERY_BF_AGGREGATION_MS`: Query-BF aggregation window (default `200`, alias `NETOS_AGGREGATION_WINDOW_MS`)
- `NETOS_QUERY_BF_TTL_MS`: Query-BF aging/TTL window (default `2000`)
- `NETOS_QUERY_BF_MAX_FILL_RATIO`: Query-BF split threshold (0-1, default `0.9`, `0` disables)
- `NETOS_QUERY_BF_MAX_KEYS`: Query-BF max keys per batch (default `0`, disabled)
- `NETOS_BROADCAST_ATTEMPT_LIMIT`: Query-BF forward attempt limit (default `3`)
- `NETOS_BROADCAST_WINDOW_MS`: Query-BF attempt window (default `1000`)
- `NETOS_ASYNC_ENABLE`: enable async forwarding (default `true`)
- `NETOS_FORWARD_WORKERS`: async forwarding worker count (default `1`)
- `NETOS_FORWARD_QUEUE_MAX`: async forwarding queue depth (default `1024`)
- `NETOS_FORWARD_DROP_POLICY`: forward queue overflow policy (default `drop_newest`)
- `NETOS_LOG_LEVEL`: `debug|info|warn|error`

Config files (via `NETOS_CONFIG_FILE` or `NETOS_TOPOLOGY_DIR`) are loaded first, and process
environment variables override any values from the file.

## Repo Layout

- `docs/TECH_STACK.md`: stack decisions and constraints
- `docs/QUICKSTART.md`: quickstart and staged demo flow
- `docs/PROTOTYPE_OVERVIEW.md`: concise prototype overview and demo narrative
- `docs/RUNBOOK.md`: operator/developer runbook and entry points
- `docs/OBSERVABILITY.md`: log fields and state labels
- `docs/DEMO_3_NODE.md`: 3-node demo walkthrough
- `design/ARCHITECTURE.md`: demo architecture summary
- `docs/ROADMAP.md`: incremental demo milestones
- `docs/HANDOFF.md`: concise handoff guide for new developers
- `src/`: C++20 implementation
- `infra/`: Docker assets (Dockerfile, compose, entrypoint)
- `infra/topology/`: demo topology env files
- `scripts/validate_demo.sh`: basic docker-compose validation path
- `scripts/validate_3node_demo.sh`: 3-node hop-through validation helper
- `scripts/check_topology_env.sh`: topology env sanity check
- `scripts/hop_story_3node.sh`: 3-node hop-through timeline + hop chain helper
