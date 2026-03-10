# NetOS Demo Prototype

Minimal runnable demo of the NetOS synchronization layer based on meeting notes. It keeps the Redis-proxy model and co-locates Redis + sync app in the same container, using a small UDP request/data exchange to validate the architecture direction.

## Quick Start (Docker Compose)

```bash
docker compose -f infra/docker-compose.yml up --build
```

You should see logs like:
- Node A seeds key `alpha`
- Node B broadcasts a request for `alpha`
- Node A serves `alpha`
- Node B stores `alpha` locally

Stop with `Ctrl+C`.

## Local Build (Optional)

```bash
cmake -S . -B build
cmake --build build -j
./build/netos-node
```

## Environment Variables

- `NETOS_NODE_ID`: node name (e.g., `node1`)
- `NETOS_BIND_IP`: UDP bind IP (default `0.0.0.0`)
- `NETOS_BIND_PORT`: UDP port (default `9000`)
- `NETOS_NEIGHBORS`: comma list of `host:port` entries
- `NETOS_REDIS_SOCKET`: Unix socket for Redis (default `/var/run/redis/redis.sock`)
- `NETOS_SEED_KEYS`: semicolon list `k=v;k2=v2`
- `NETOS_REQUEST_KEYS`: comma list of keys to request at startup
- `NETOS_REQUEST_DELAY_MS`: delay before requesting keys
- `NETOS_REQUEST_TTL`: request TTL (default `3`)
- `NETOS_QUERY_TTL_MS`: QueryTable TTL (default `1500`)
- `NETOS_SYNC_TABLE_CAPACITY`: SyncTable capacity (default `128`)
- `NETOS_LOG_LEVEL`: `debug|info|warn|error`

## Repo Layout

- `docs/TECH_STACK.md`: stack decisions and constraints
- `design/ARCHITECTURE.md`: demo architecture summary
- `docs/ROADMAP.md`: incremental demo milestones
- `src/`: C++20 implementation
- `infra/`: Docker assets (Dockerfile, compose, entrypoint)
