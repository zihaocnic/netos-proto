# NetOS Demo Architecture

This document summarizes the **minimal runnable demo** architecture derived from the meeting notes. It implements the same direction (Redis-backed cache + external synchronization agent) while keeping the scope intentionally small.

## Architecture Snapshot

```
+-----------------------------+        UDP request/data         +-----------------------------+
| Node A container            | <------------------------------> | Node B container            |
|                             |                                  |                             |
|  netos-node (sync app)      |                                  |  netos-node (sync app)      |
|  - Node / Message           |                                  |  - Node / Message           |
|  - QueryTable (TTL)         |                                  |  - QueryTable (TTL)         |
|  - SyncTable (LRU stub)     |                                  |  - SyncTable (LRU stub)     |
|  - UDP Network (send/bcast) |                                  |  - UDP Network (send/bcast) |
|                             |                                  |                             |
|  Redis 7 (unix socket)      |                                  |  Redis 7 (unix socket)      |
+-----------------------------+                                  +-----------------------------+
```

Key alignment points:
- **Redis + Sync App in the same container**, communicating through a Unix socket.
- **User-space** implementation with simple UDP messaging.
- The synchronization logic is **outside Redis** (proxy mode).

## Demo Message Flow

1. Node B boots and issues a `REQUEST` for key `alpha`.
2. Node B **broadcasts** the request to neighbors (Node A).
3. Node A finds `alpha` in its local Redis and sends `DATA` back to Node B.
4. Node B stores the value into its local Redis.

This models the early **Pull** phase in the architecture notes.

## Request/Data State Decisions (Demo)

The demo keeps the request/data handling path state-machine-like with explicit decision states:
- **Request**: `drop_invalid` → `drop_ttl` → `drop_duplicate` → `serve_local` → `forward`
- **Data**: `drop_invalid` → `drop_not_origin` → `store_local`

Contract notes (Phase 1):
- `origin` is always the request origin for both `REQ` and `DATA`.
- `DATA` is stored only by the request origin and is not forwarded.
- `DATA` TTL is a sanity check only (not decremented).

See `docs/CONTRACTS.md` for the full behavior contract and non-guarantees.

## Components (Minimal)

- **Node**: Core orchestration. Loads config, listens for messages, and triggers requests.
- **Message**: Simple wire format (`REQ|id|origin|ttl|key|value`).
- **QueryTable**: In-memory TTL map of **request IDs** to suppress duplicate/looping requests. It does not store keys/values.
- **SyncTable**: Small LRU map (stub) of **key → origin IDs** for requests this node served (future push/subscription hook).
- **Network API**: UDP send/broadcast (neighbors list = demo topology).
- **Cache**: Redis 7 accessed via Unix domain socket.

## Phase 1 Scope Mapping (Meeting Notes)

Phase 1 implements the minimal Pull loop from the meeting-note architecture while keeping module boundaries aligned.

- **Node + local cache**: implemented (`netos-node` + Redis unix socket).
- **QueryTable**: implemented (TTL suppression).
- **SyncTable**: implemented as an LRU stub for destination tracking.
- **Control-plane messaging**: UDP `REQ`/`DATA` only.
- **Topology management**: static neighbor list (env-file topology).
- **Push, Bloom filters, async forwarding**: explicitly out of scope for Phase 1.

## Gaps vs Target Architecture

These are intentionally out of scope for the demo but remain aligned with the direction:
- Bloom Filter exchange and periodic BF broadcasts.
- Secondary BF and more robust loop prevention.
- Buffer / async forwarding process separation.
- Push phase (subscription-based replication).
- Kathara/NS-3 scale testing.

## Why This Fits the Direction

- Keeps the Redis proxy model (no Redis source changes).
- Preserves the “router + cache” co-deployment model.
- Maintains modular C++ components that match the Node/Packet/QueryTable/SyncTable/Network abstractions.
