# NS-3 Scaffold for NetOS

This is a minimal scaffold to export NetOS topology configs into a simple JSON format that can be consumed by NS-3 scripts. It does not run NS-3 and does not introduce any C++ changes.

## Files
- `scripts/ns3/generate_topology.py`
- `docs/NS3.md`

## Topology Export

Generate JSON from a specific topology directory:

```bash
./scripts/ns3/generate_topology.py --input infra/topology/3-node --output /tmp/netos-topology.json
```

Generate JSON for all topologies under `infra/topology`:

```bash
./scripts/ns3/generate_topology.py --input infra/topology --output /tmp/netos-topology.json
```

If you pass a single env file, it will export only that file's node plus its declared neighbors.

### JSON Schema (netos-ns3-topology-v1)

```json
{
  "schema": "netos-ns3-topology-v1",
  "source": { "input": "infra/topology" },
  "topologies": [
    {
      "name": "3-node",
      "env_files": ["infra/topology/3-node/node1.env"],
      "nodes": [
        { "id": "node1", "bind_ip": "0.0.0.0", "bind_port": 9001, "config_file": "..." }
      ],
      "links": [
        { "src": "node1", "dst": "node2", "dst_port": 9002 }
      ]
    }
  ]
}
```

Notes:
- Links are directional, following the `NETOS_NEIGHBORS` list. For undirected links, treat reciprocal entries as a single link in NS-3.
- `bind_port` and `dst_port` are parsed as integers when possible.

## Mapping Notes (NetOS → NS-3)

Topology:
- `NETOS_NODE_ID` maps to NS-3 node names/IDs.
- `NETOS_BIND_IP` and `NETOS_BIND_PORT` map to the NetOS app bind address/port in NS-3.
- `NETOS_NEIGHBORS` defines adjacency; each entry becomes a point-to-point (or CSMA) link.

Latency:
- No per-link latency is currently expressed in NetOS env files.
- For scaffolding, use a fixed default (for example 1ms) in NS-3, or add a sidecar mapping file for link delays.

Loss:
- No per-link loss is currently expressed in NetOS env files.
- For scaffolding, use a default loss model in NS-3 or add a sidecar mapping file for per-link loss.

Traffic patterns:
- `NETOS_SEED_KEYS` defines initial cached keys (data sources).
- `NETOS_REQUEST_KEYS` defines which keys a node requests.
- `NETOS_REQUEST_DELAY_MS` defines the request start time.
- `NETOS_REQUEST_TTL` can map to packet TTL or an application-level hop limit.
- `NETOS_QUERY_TTL_MS` can map to an application-level request timeout window.

## Scope

This scaffold only exports topology metadata. NS-3 simulation code and workload drivers are intentionally out of scope for now.
