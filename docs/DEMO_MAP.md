# NetOS Demo Map

A compact index for presentation paths, demo helpers, and repo entry points.

## Presentation Paths
1. 5-minute pass: `./scripts/demo.sh trace` then `./scripts/demo.sh hop-story --3-node`.
2. 15-minute walkthrough: `./scripts/demo.sh validate` then `./scripts/demo.sh trace` then `./scripts/demo.sh pull-path --3-node` then `./scripts/demo.sh hop-story --3-node`.
3. 30-minute deep dive: `./scripts/demo.sh validate --3-node` then `./scripts/demo.sh trace --3-node` then `./scripts/demo.sh pull-path --3-node` then `./scripts/demo.sh hop-story --3-node` then `./scripts/demo.sh inspect --3-node` then `./scripts/demo.sh edge-cases`.

## Demo Driver Index
| Need | Command | Notes |
| --- | --- | --- |
| Start the stack | `./scripts/demo.sh start` | Use `--3-node` or `--compose` for alternate topologies. |
| Validate | `./scripts/demo.sh validate` | Add `--3-node` for the hop-through demo. |
| Trace request/data states | `./scripts/demo.sh trace` | Prints `req_state` / `data_state` lines and timeline. |
| Summarize table stats | `./scripts/demo.sh table-stats` | Prints latest QueryTable/SyncTable stats per node. |
| Table stats health check | `./scripts/demo.sh table-health` | Derives health from the table stats summary. |
| Pull-path summary | `./scripts/demo.sh pull-path --3-node` | Best on the 3-node topology. |
| Hop-through story | `./scripts/demo.sh hop-story --3-node` | Intended for the 3-node demo. |
| Inspect Redis state | `./scripts/demo.sh inspect` | Add `--inspect-keys k1,k2` to check multiple keys. |
| Edge-case injection | `./scripts/demo.sh edge-cases` | Uses the 2-node stack by default. |
| Check topology envs | `./scripts/demo.sh check-topology` | Pass extra topology dirs as arguments. |

## Helper Scripts (Direct)
The demo driver wraps these scripts. Run them directly if you need a single helper.

| Helper | Purpose |
| --- | --- |
| `scripts/validate_demo.sh` | 2-node validation pass/fail. |
| `scripts/validate_3node_demo.sh` | 3-node validation pass/fail. |
| `scripts/trace_demo.sh` | Trace request/data states + timeline. |
| `scripts/table_stats_demo.sh` | QueryTable/SyncTable stats summary. |
| `scripts/pull_path_demo.sh` | Pull-path summary (`origin -> served_by -> stored_at`). |
| `scripts/hop_story_3node.sh` | Hop-through timeline + story beats (3-node). |
| `scripts/inspect_demo.sh` | Redis state snapshot after the key lands. |
| `scripts/observe_edge_cases.sh` | Inject malformed/expired/not-origin messages. |
| `scripts/check_topology_env.sh` | Validate topology env files. |

## Docs Map
| Doc | Use It For |
| --- | --- |
| `docs/QUICKSTART.md` | Staged demo flow and fast setup. |
| `docs/PROTOTYPE_OVERVIEW.md` | Architecture summary and scope limits. |
| `docs/RUNBOOK.md` | Operator commands and troubleshooting. |
| `docs/HANDOFF.md` | New developer or meeting host handoff. |
| `docs/CONTRACTS.md` | Phase-1 behavior guarantees. |
| `docs/OBSERVABILITY.md` | Log fields and state labels. |
| `docs/DEMO_3_NODE.md` | 3-node hop-through expectations. |
| `design/ARCHITECTURE.md` | Architecture snapshot diagram. |

## Common Overrides
- `--3-node` or `COMPOSE_FILE=infra/docker-compose.3-node.yml` for the hop-through demo.
- `--key KEY` or `EXPECT_KEY=KEY` to change the expected key.
- `--timeout SECONDS` or `TIMEOUT_SEC=SECONDS` to extend helper timeouts.
- `--compose FILE` and `--project NAME` to avoid compose collisions.

## Validation Checklist
- `./scripts/demo.sh check-topology`
- `./scripts/demo.sh validate`
- `./scripts/demo.sh validate --3-node`
- `ctest --test-dir build --output-on-failure` after a local build
