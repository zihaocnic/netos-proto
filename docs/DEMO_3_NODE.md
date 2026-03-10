# 3-Node Demo (Linear Hop-Through)

This demo adds a simple 3-node linear topology: node1 <-> node2 <-> node3.
Node1 seeds the key `alpha`. Node2 requests `alpha` first and caches it. Node3
requests `alpha` later and gets it from node2. This shows a hop-through pull
where the data originates at node1 and reaches node3 via node2's cache.

## Run (Docker Compose)

```bash
docker compose -f infra/docker-compose.3-node.yml up --build
```

Stop with `Ctrl+C`.

## Quick Validation

```bash
./scripts/validate_3node_demo.sh
```

The helper waits for these events:
- node1 logs `req_state=serve_local` with `origin=node2`
- node2 logs `data_state=store_local` for `alpha`
- node2 logs `req_state=serve_local` with `origin=node3`
- node3 logs `data_state=store_local` for `alpha`

## Topology Files

- `infra/topology/3-node/node1.env`
- `infra/topology/3-node/node2.env`
- `infra/topology/3-node/node3.env`

The request delays are staggered so node2 fetches the key before node3 requests it.
