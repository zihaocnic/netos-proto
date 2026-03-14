# NetOS Demo Observability

This guide documents the current log fields and state labels for the Phase-1 + Phase-2.3 demo.

## Log Format

Each log line is:

`YYYY-MM-DD HH:MM:SS [LEVEL] message`

## Network API (Phase 2.1)

- `send_direct(addr, msg)` sends a single UDP message to the provided socket address and returns `false` with an error string on transport failure.
- `send_broadcast(msg, exclude)` sends the message to every neighbor in the loaded topology and skips the `exclude` address when provided (used to avoid echoing back to the sender).
- Topology is built at startup from config: `NETOS_CONFIG_FILE` selects a specific env file, otherwise `NETOS_TOPOLOGY_DIR` + `NETOS_NODE_ID` picks `node_id.env`, and `NETOS_NEIGHBORS` (env or env-file) supplies the neighbor list.
- Neighbor address resolution is deferred to send time to avoid startup failures when DNS entries are not yet ready inside containers.
- This API adoption does not change request/data behavior or logging; existing `req_state=`/`data_state=` fields and `send failed to ...` warnings remain unchanged.

## Startup and Config Logs

- `config node_id=... source=... bind=... neighbors=... seed_keys=... request_keys=... request_delay_ms=... request_ttl=... query_ttl_ms=... sync_table_capacity=... content_bf_bits=... content_bf_hashes=... content_bf_exchange_ms=... content_bf_ttl_ms=... content_bf_fallback_ms=... query_bf_bits=... query_bf_hashes=... query_bf_aggregation_ms=... query_bf_ttl_ms=... broadcast_attempt_limit=... broadcast_window_ms=... log_level=...`
- `node <id> listening on <ip>:<port>`
- `seeded key <key>`

## Request Path (`req_state=`)

State labels:
- `originated` (node issues a request at startup)
- `drop_invalid` (missing request_id, origin, or key)
- `drop_ttl` (ttl <= 0, `reason=ttl_expired`)
- `drop_duplicate` (duplicate request_id within QueryTable TTL)
- `drop_suppressed` (Query-BF aggregation/forwarding suppressed; see `reason=...`)
- `serve_local` (key found locally, response sent)
- `serve_failed` (response send failed)
- `forward` (broadcast to neighbors with ttl-1)

Common fields (when present):
- `id` request_id
- `key` key name
- `ttl` request ttl
- `ttl_in` inbound ttl (forward logs only)
- `origin` request origin node id
- `from` source address of the inbound request (`from=local` for originated)
- `dest` address a response was sent to
- `bf_type` `query` when the log line refers to Query-BF handling
- `bf_id` Query-BF batch id (present when `bf_type=query`)
- `bf_age_ms` Query-BF age in milliseconds (present when `bf_type=query`)
- `query_table_attempts` total request IDs recorded (QueryTable)
- `query_table_duplicates` total duplicate request IDs seen (QueryTable)
- `query_table_pruned` total expired IDs pruned (QueryTable)
- `query_table_size` current QueryTable size

QueryTable fields are emitted alongside `req_state=originated`, `req_state=serve_local`,
and `req_state=drop_duplicate` to give a running view of duplicate suppression.

Reason values (when present):
- `drop_invalid`: `missing_request_id`, `missing_origin`, `missing_key`
- `drop_invalid` (Query-BF): `missing_bloom`, `missing_created_ms`, `bad_created_ms`, `bad_bloom`
- `drop_ttl`: `ttl_expired`
- `drop_suppressed`: `aggregate_window`, `attempt_limit`, `bf_expired`, `content_fallback`

## Data Path (`data_state=`)

State labels:
- `drop_invalid` (missing request_id/origin/key or ttl <= 0)
- `drop_not_origin` (data delivered to a node that did not originate the request)
- `store_local` (value stored in local Redis)
- `store_failed` (Redis write failed)

Common fields (when present):
- `id` request_id
- `key` key name
- `ttl` data ttl
- `origin` request origin node id
- `from` source address of the inbound data
- `at` node id performing drop_not_origin

Reason values (when present):
- `drop_invalid`: `missing_request_id`, `missing_origin`, `missing_key`, `non_positive_ttl`

## Bloom Filter Logs

Content-BF (neighbor cache summaries):
- `bf_state=content_send` (local summary broadcast; debug-level)
- `bf_state=content_update` (neighbor summary received)
- `bf_state=content_drop` (invalid Content-BF payload)
- `bf_state=content_direct` (local miss routed to a neighbor hit)
- `bf_state=content_direct_failed` (direct query send failed)

Common Content-BF fields:
- `origin` summary origin node id
- `from` sender address
- `neighbor` selected direct-query neighbor
- `bf_bits`, `bf_hashes`, `bf_bytes` Bloom filter parameters
- `neighbor_summaries` count of active neighbor summaries

Query-BF (broadcast query aggregation):
- Query-BF actions reuse `req_state=` and include `bf_type=query`.
- `bf_id` identifies the aggregated batch (origin + aggregation window).
- `bf_age_ms` reports the age of the Query-BF used for forwarding/suppression.
- `bf_bits`, `bf_hashes` identify the Query-BF parameters.

## Pipeline Stages (Decision Order)

The request/data logs are emitted after the pipeline decision returns in `src/netos/node/node.cpp`.
These stages are intentionally ordered and map directly to the state labels above.

Request pipeline (`src/netos/node/request_pipeline.cpp`, `src/netos/node/request_pipeline.h`):
- validate required fields → `req_state=drop_invalid` (`missing_*`)
- validate TTL (`ttl <= 0`) → `req_state=drop_ttl` (`ttl_expired`)
- QueryTable dedupe → `req_state=drop_duplicate` (`duplicate_request_id`)
- Redis cache lookup → `req_state=serve_local` on hit
- forward miss (`ttl-1`) → `req_state=forward`
If a `serve_local` response send fails, `src/netos/node/node.cpp` logs `req_state=serve_failed`.

Data pipeline (`src/netos/node/data_pipeline.cpp`, `src/netos/node/data_pipeline.h`):
- validate required fields / TTL → `data_state=drop_invalid` (`non_positive_ttl`)
- check request origin matches `node_id` → `data_state=drop_not_origin`
- store locally → `data_state=store_local`

Query-BF handling (`src/netos/node/node.cpp`, `handle_query_bloom`):
- validate required fields → `req_state=drop_invalid` (`missing_*` / `bad_*`)
- validate TTL/age → `req_state=drop_ttl` or `req_state=drop_suppressed` (`bf_expired`)
- local cache match (multi-hit) → `req_state=serve_local` (`bf_type=query`) per matching key
- forward Query-BF (`ttl-1`) with attempt limit → `req_state=forward` or `req_state=drop_suppressed`

## Field Semantics Notes

- `origin` always refers to the request origin, for both `REQ` and `DATA`.
- `from` is the immediate sender address (or `from=local` for originated requests).
- `DATA` TTL is a sanity check only and is not decremented or forwarded.

## Table Semantics Notes (Phase 2.1)

- QueryTable is single-source: `request_id` alone defines uniqueness, so repeats are dropped even if they come from different origins (no multi-requester coalescing).
- SyncTable keys are file names (the request `key`), with LRU eviction by key when capacity is exceeded.
- No SYNC packet: SyncTable state updates on DATA receipt, per meeting decisions.

## Other Logs (Non-state)

- `received malformed message from ...`
- `received unknown message type from ...`
- `send failed to ...`
- `redis get error: ...`
- `seed failed for key ...`
- `sync_table=update key=... origin=... new_key=... new_destination=... duplicate_destination=... destinations=... size=... evicted=... sync_table_updates=... sync_table_duplicate_hits=... sync_table_destinations_total=... sync_table_evicted_total=...`

## Wire Format (Demo)

- `REQ|request_id|origin|ttl|key`
- `DATA|request_id|origin|ttl|key|value`
- `CBF|request_id|origin|ttl|bloom_hex`
- `QBF|request_id|origin|ttl|bloom_hex|created_ms`

`value` may be empty. The parser accepts `REQ` with an optional empty value field (5 or 6 tokens),
and `ttl` is decremented when forwarding. Content-BF payloads use the `key` slot (`bloom_hex`) and
may leave `request_id` empty; Query-BF payloads include `bloom_hex` plus a millisecond creation
timestamp and use `request_id` as the `bf_id`.

## Handy Grep

```bash
docker compose -f infra/docker-compose.yml logs --no-color | grep -E "req_state=|data_state="
```

```bash
docker compose -f infra/docker-compose.3-node.yml logs --no-color node2 | grep "req_state=serve_local"
```

`trace_demo.sh` and `inspect_demo.sh` already wrap these patterns for the 2-node demo.
`trace_demo.sh` also prints a compact per-event timeline to make state transitions easier
to narrate during demos.

## Table Stats Summary Helper

```bash
./scripts/table_stats_demo.sh
```

This helper starts the demo, waits for `data_state=store_local`, and prints the latest
QueryTable and SyncTable stats per node based on the `query_table_*` and `sync_table_*`
log fields. The summary reports `sync_table_keys` derived from the `size=` field in the
`sync_table=update` log line.

The default output now includes a concise per-node summary line, e.g.
`QueryTable: size=... dup=... pruned=...; SyncTable: keys=... updates=... evicted=...`.
If a node has not emitted any SyncTable updates yet, the concise summary uses
zero placeholders for the SyncTable fields to keep per-node lines consistent.

For a compact single-line summary per node, set `TABLE_STATS_FORMAT=compact` (or `summary`)
when running the helper:

```bash
./scripts/demo.sh table-stats --table-format compact
```

For a one-line health summary derived from the table-stats output, use:

```bash
./scripts/table_stats_health.sh
```

This emits a per-node line like:
`node1 QueryTable healthy: dup=0, pruned=0; SyncTable healthy: evicted=0`.

## Pull Path Summary Helper

```bash
./scripts/pull_path_demo.sh
```

This helper derives a compact `origin -> served_by -> stored_at` summary for each request ID
matching the expected key, plus a short pull-story narrative per request. Use
`COMPOSE_FILE=infra/docker-compose.3-node.yml` for the 3-node demo. Node labels reflect the
docker compose service names.

## Hop-Through Story Helper (3-Node)

```bash
./scripts/hop_story_3node.sh
```

This helper prints a step-by-step timeline, story beats, and hop chain summary for the
3-node linear hop-through story.
