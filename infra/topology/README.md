# Demo Topology Files

This folder stores explicit topology configs for the Docker Compose demo.

## Current Topology

- `2-node/` contains the default two-node demo.
- `node1.env` and `node2.env` are loaded by `infra/docker-compose.yml` via `env_file`.

## Extending the Topology

1. Copy `infra/topology/2-node` to a new folder (for example `4-node`).
2. Add new `nodeN.env` files with unique `NETOS_NODE_ID` and `NETOS_BIND_PORT` values.
3. Update `NETOS_NEIGHBORS` lists to reflect the desired graph.
4. Duplicate a service in `infra/docker-compose.yml` and point its `env_file` at the new `.env` file.

Keep each node's configuration self-contained so topology changes are easy to review and version.
