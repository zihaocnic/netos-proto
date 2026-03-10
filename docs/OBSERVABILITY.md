# NetOS Demo Observability

This guide documents the current log fields and state labels for the Phase-1 demo.

## Log Format

Each log line is:

`YYYY-MM-DD HH:MM:SS [LEVEL] message`

## Startup and Config Logs

- `config node_id=... source=... bind=... neighbors=... seed_keys=... request_keys=... request_delay_ms=... request_ttl=... query_ttl_ms=... sync_table_capacity=... log_level=...`
- `node <id> listening on <ip>:<port>`
- `seeded key <key>`

## Request Path (`req_state=`)

State labels:
- `originated` (node issues a request at startup)
- `drop_invalid` (missing request_id, origin, or key)
- `drop_ttl` (ttl <= 0, `reason=ttl_expired`)
- `drop_duplicate` (duplicate request_id within QueryTable TTL)
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

Reason values (when present):
- `drop_invalid`: `missing_request_id`, `missing_origin`, `missing_key`
- `drop_ttl`: `ttl_expired`

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

## Field Semantics Notes

- `origin` always refers to the request origin, for both `REQ` and `DATA`.
- `from` is the immediate sender address (or `from=local` for originated requests).
- `DATA` TTL is a sanity check only and is not decremented or forwarded.

## Other Logs (Non-state)

- `received malformed message from ...`
- `received unknown message type from ...`
- `send failed to ...`
- `redis get error: ...`
- `seed failed for key ...`

## Wire Format (Demo)

- `REQ|request_id|origin|ttl|key`
- `DATA|request_id|origin|ttl|key|value`

`value` may be empty. The parser accepts `REQ` with an optional empty value field (5 or 6 tokens),
and `ttl` is decremented when forwarding.

## Handy Grep

```bash
docker compose -f infra/docker-compose.yml logs --no-color | grep -E "req_state=|data_state="
```

```bash
docker compose -f infra/docker-compose.3-node.yml logs --no-color node2 | grep "req_state=serve_local"
```

`trace_demo.sh` and `inspect_demo.sh` already wrap these patterns for the 2-node demo.

## Pull Path Summary Helper

```bash
./scripts/pull_path_demo.sh
```

This helper derives a compact `origin -> served_by -> stored_at` summary for each request ID
matching the expected key. Use `COMPOSE_FILE=infra/docker-compose.3-node.yml` for the 3-node
demo. Node labels reflect the docker compose service names.
