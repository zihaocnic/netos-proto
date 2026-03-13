# NetOS Kathara Demo (3-Node Linear)

This document describes the Kathara-based 3‑node linear topology demo for the NetOS prototype.

## Topology

### 3‑node linear
```
node1 <-> node2 <-> node3
```

- node1 seeds `alpha`
- node2 requests `alpha` first
- node3 requests `alpha` later (hop-through via node2)

### 4‑node hub
```
node1 -- node2 -- node3
             |
           node4
```

- node1 seeds `alpha`
- node2 requests `alpha`
- node3 + node4 request `alpha` later (hop-through via node2)

## Files

- `infra/kathara/3-node/lab.conf`
- `infra/kathara/3-node/node1.startup`
- `infra/kathara/3-node/node2.startup`
- `infra/kathara/3-node/node3.startup`
- `infra/kathara/4-node-hub/lab.conf`
- `infra/kathara/4-node-hub/node{1,2,3,4}.startup`
- `infra/kathara/4-node-hub/README.md`
- `infra/Dockerfile.kathara`

## Prerequisites

- Kathara installed and verified (`python -m kathara check`)
- Docker available (Kathara uses Docker backend)

## Build Image

This demo uses a dedicated image built from the repo:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

docker build -f infra/Dockerfile.kathara -t netos-demo:latest .
```

## Start Demo

```bash
# 3-node linear
python -m kathara lstart --noterminals -d infra/kathara/3-node

# 4-node hub
python -m kathara lstart --noterminals -d infra/kathara/4-node-hub
```

## Verify Demo

Look for these log lines in each node’s `/var/log/startup.log`:

**3-node linear**
- node1 should **serve_local** to node2
- node2 should **store_local** for `alpha`
- node3 should **store_local** for `alpha` (hop-through)

**4-node hub**
- node1 should **serve_local** to node2
- node2 should **store_local** for `alpha`
- node3 + node4 should **store_local** for `alpha` (hop-through via node2)

Example verification commands:

```bash
# 3-node linear
python -m kathara exec node1 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'
python -m kathara exec node2 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'
python -m kathara exec node3 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'

# 4-node hub
python -m kathara exec node1 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 10'
python -m kathara exec node2 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 40'
python -m kathara exec node3 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'
python -m kathara exec node4 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'
```

## Stop Demo

```bash
python -m kathara lclean -d infra/kathara/3-node
python -m kathara lclean -d infra/kathara/4-node-hub
```

## Notes

- This scenario uses static IPs:
  - node1: 10.0.12.1/24 (eth0)
  - node2: 10.0.12.2/24 (eth0), 10.0.23.2/24 (eth1)
  - node3: 10.0.23.3/24 (eth0)
- Hostname resolution is injected via `/etc/hosts` in each startup script.
- If your environment uses a GUI, you can omit `--noterminals` to open xterm windows.
