# NetOS Demo Tech Stack

This demo aligns to the meeting notes and the living architecture docs while keeping scope minimal.

## Stack Summary
- **Language**: C++20 (primary implementation language)
- **Build**: CMake 3.16+
- **Cache backend**: Redis 7 (proxy mode, no Redis Cluster)
- **Containerization**: Docker + docker-compose
- **Inter-node messaging (demo)**: UDP (control-plane style)
- **Local cache access**: Unix domain socket to Redis

## Rationale From Notes
- C++20 is preferred for performance and modern language features.
- Redis is the initial single-node cache; synchronization logic stays **outside** Redis in the first version.
- Redis + Sync App should be **co-located in the same container** using a Unix socket to minimize latency.
- The first prototype runs in **user space** and uses standard protocols (TCP/UDP) for control.
- Dragonfly is a potential alternative KV store but remains a later evaluation.

## Demo-Specific Choices
- UDP is used for the demo's request/data messages to keep the networking light.
- Redis is started with **Unix socket only** (TCP disabled) to match the intended deployment direction.
- The demo uses a minimal **QueryTable** and **SyncTable** skeletons to stay aligned with the architectural direction.

## Constraints and Guardrails
- Do not modify Redis source code in the demo.
- Keep Redis non-clustered; synchronization is handled by the NetOS layer.
- Keep code modular with clear components (Node, Packet/Message, QueryTable, SyncTable, Network, Cache).
- Avoid Python for core data path logic in this prototype.
