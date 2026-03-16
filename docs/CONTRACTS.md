# Phase-1 Behavior Contracts

This document records what the current NetOS demo prototype does and does not guarantee.
It is meant to keep Phase-1 behavior predictable while the implementation stays small.

## Guaranteed Behaviors (Phase 1)

- Requests with missing `request_id`, `origin`, or `key` log `req_state=drop_invalid`.
- Requests with `ttl <= 0` log `req_state=drop_ttl` with `reason=ttl_expired`.
- Requests with a duplicate `request_id` within the QueryTable TTL log `req_state=drop_duplicate`.
- Cache hits reply with `DATA` to the sender and log `req_state=serve_local`.
- Cache misses forward the request to neighbors with `ttl-1` and log `req_state=forward`.
- Data with missing `request_id`, `origin`, or `key`, or with `ttl <= 0`, logs `data_state=drop_invalid`.
- Data delivered to a node that did not originate the request logs `data_state=drop_not_origin`.
- Data delivered to the request origin is stored locally and logs `data_state=store_local`.

## Message Semantics

- `origin` always refers to the **request origin**, for both `REQ` and `DATA`.
- `DATA` does not carry the responder identity. Use the `from=` log field for the immediate sender.
- `NETOS_REQUEST_TTL` is used for outbound requests and for response `DATA` TTL.
- `DATA` TTL is a sanity check only and is not decremented or forwarded.

## QueryTable Contract

- QueryTable tracks **request IDs only** for TTL-based duplicate suppression.
- It does not dedupe by key and does not persist across restarts.

## SyncTable Contract

- SyncTable records which request origins asked for keys this node served.
- It is a bounded LRU stub and can evict entries at any time.
- It does not affect request or data routing in Phase 1.

## Redis Interaction Contract

- Redis `GET` errors are logged and treated as cache misses.
- Redis `SET` errors are logged as `data_state=store_failed`.
- There are no retries, backoffs, or reconnection attempts in the data path.

## Topology Contract

- Neighbors are loaded from environment or config files at startup.
- When `NETOS_TOPOLOGY_RELOAD_MS` is non-zero, the node periodically reloads the env config and updates its neighbor list (best-effort, no protocol messages).
- Requests are broadcast to all neighbors except the immediate sender.
- An empty neighbor list means forwards become no-ops.

## Async Forwarding (Phase 3.3)

- Outbound direct + broadcast messages pass through the async forwarding queue when enabled.
- Queue overflow drops the newest message (`NETOS_FORWARD_DROP_POLICY=drop_newest`) and logs `forward_state=drop_queue`.

## Propagation Control (Phase 2.3)

- Push stage is the periodic Content-BF (CBF) summary exchange (local cache -> neighbors); no Interest packets are sent.
- Local misses first consult neighbor Content-BF summaries; direct queries are attempted when a neighbor
  indicates a possible hit.
- Content-BF summaries are exchanged every `NETOS_CONTENT_BF_EXCHANGE_MS` and expire after
  `NETOS_CONTENT_BF_TTL_MS`.
- Content-BF direct queries that do not yield data within `NETOS_CONTENT_BF_FALLBACK_MS` schedule the
  key into Query-BF aggregation; the fallback is canceled if data is stored before the window expires.
- If no Content-BF hit is found, keys are aggregated into a per-origin Query-BF within
  `NETOS_QUERY_BF_AGGREGATION_MS` (alias `NETOS_AGGREGATION_WINDOW_MS`); suppressed forwards
  log `req_state=drop_suppressed` with `reason=aggregate_window`.
- Query-BF aggregation is per-origin only (no cross-origin merges).
- Query-BF aggregation splits early when a batch exceeds `NETOS_QUERY_BF_MAX_FILL_RATIO` or
  `NETOS_QUERY_BF_MAX_KEYS` (when enabled); splits log `bf_state=query_split`.
- Query-BF handling checks all local keys; every matching key responds with `DATA`, and the Query-BF
  is still forwarded with `ttl-1` subject to attempt limits.
- Query-BF forwards are capped by `NETOS_BROADCAST_ATTEMPT_LIMIT` within `NETOS_BROADCAST_WINDOW_MS`;
  suppressed forwards log `req_state=drop_suppressed` with `reason=attempt_limit`.
- Query-BF forwarding decrements `ttl` and is subject to BF aging/TTL policies
  (`NETOS_QUERY_BF_TTL_MS`); expired Query-BFs log `req_state=drop_suppressed` with
  `reason=bf_expired`.
- Propagation control is best-effort and does not guarantee that every request is forwarded.

## Non-Guarantees (Explicitly Out of Scope)

- Reliable delivery, ordering, or exactly-once semantics.
- Cross-node global uniqueness of `request_id` across restarts or clock skew.
- Subscription-based Push pipeline behavior.
- Dynamic topology discovery or protocol-driven membership changes.
