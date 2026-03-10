# NetOS Demo Prototype

Minimal runnable demo of the NetOS synchronization layer based on meeting notes. It keeps the Redis-proxy model and co-locates Redis + sync app in the same container, using a small UDP request/data exchange to validate the architecture direction.

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

## Phase 1 Scope (Meeting-Note Mapping)

Phase 1 targets the minimal **Pull** loop described in the meeting notes. It keeps module boundaries aligned with the long-term architecture while staying intentionally small.

- Node + local cache: implemented (`netos-node` + Redis unix socket).
- QueryTable duplicate suppression: implemented (TTL map).
- SyncTable destination tracking: implemented as a small LRU stub.
- Control-plane messaging: UDP `REQ`/`DATA` only.
- Topology management: static neighbor list via per-node env files.
- Push pipeline, Bloom filters, async forwarding: out of scope for Phase 1.

## Table Semantics (Demo)

- **QueryTable**: TTL map of **request IDs** used to suppress duplicate/looping requests.
- **SyncTable**: LRU map of **key → origin node IDs** for requests this node served (stub for future push).

## Local Build (Optional)

```bash
cmake -S . -B build
cmake --build build -j
./build/netos-node
```

## Validation (Docker Compose)

```bash
./scripts/validate_demo.sh
```

This script brings the demo up, waits for the `data_state=store_local` log line for `alpha`, and tears the stack down.

## Trace Helper

```bash
./scripts/trace_demo.sh
```

This helper runs the demo and prints the request/data state trace lines for the expected key.

## Inspection Helper

```bash
./scripts/inspect_demo.sh
```

This helper runs the demo, waits for the expected key to land, and prints per-node Redis
state (db size, keys, and selected key values) plus the config summary line.
Use `INSPECT_KEYS=alpha,beta` to check multiple keys in one run.

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
- `NETOS_LOG_LEVEL`: `debug|info|warn|error`

Config files (via `NETOS_CONFIG_FILE` or `NETOS_TOPOLOGY_DIR`) are loaded first, and process
environment variables override any values from the file.

## Repo Layout

- `docs/TECH_STACK.md`: stack decisions and constraints
- `design/ARCHITECTURE.md`: demo architecture summary
- `docs/ROADMAP.md`: incremental demo milestones
- `src/`: C++20 implementation
- `infra/`: Docker assets (Dockerfile, compose, entrypoint)
- `infra/topology/`: demo topology env files
- `scripts/validate_demo.sh`: basic docker-compose validation path
