# Demo Topology Files

This folder stores explicit topology configs for the Docker Compose demo.

## Current Topology

- `2-node/` contains the default two-node demo.
- Each `nodeN.env` file is a self-contained node config (same keys as process env).
- `infra/docker-compose.yml` mounts the topology folder and sets `NETOS_TOPOLOGY_DIR`.
  Each service passes `NETOS_NODE_ID` so the node loads `NODE_ID.env`.

## Extending the Topology

1. Copy `infra/topology/2-node` to a new folder (for example `4-node`).
2. Add new `nodeN.env` files with unique `NETOS_NODE_ID` and `NETOS_BIND_PORT` values.
3. Update `NETOS_NEIGHBORS` lists to reflect the desired graph.
4. Update `infra/docker-compose.yml` to point `NETOS_TOPOLOGY_DIR` at the new folder.
5. Add a service per node with `NETOS_NODE_ID` pointing at the matching file.

Keep each node's configuration self-contained so topology changes are easy to review and version.

For local runs outside Docker Compose, you can point `NETOS_CONFIG_FILE` directly at a
`nodeN.env` file instead of using `NETOS_TOPOLOGY_DIR`.
