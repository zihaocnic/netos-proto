#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-edge}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
TARGET_SERVICE="${TARGET_SERVICE:-node1}"
TARGET_PORT="${TARGET_PORT:-9001}"
SENDER_SERVICE="${SENDER_SERVICE:-node2}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this edge-case helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this edge-case helper." >&2
  exit 1
fi

echo "Starting NetOS demo via docker compose..."
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" up --build -d

cleanup() {
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" down -v >/dev/null 2>&1 || true
}
trap cleanup EXIT

deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
  if echo "$logs" | grep -q "listening on"; then
    break
  fi
  sleep 1
done

if [ $SECONDS -ge $deadline ]; then
  echo "FAIL: nodes did not reach listening state within ${TIMEOUT_SEC}s." >&2
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color || true
  exit 1
fi

send_udp() {
  local payload="$1"
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$SENDER_SERVICE" \
    env PAYLOAD="$payload" TARGET_HOST="$TARGET_SERVICE" TARGET_PORT="$TARGET_PORT" \
    bash -lc 'printf "%s" "$PAYLOAD" > /dev/udp/${TARGET_HOST}/${TARGET_PORT}'
}

invalid_payload="REQ||${SENDER_SERVICE}|2|alpha"
ttl_payload="REQ|edge-ttl|${SENDER_SERVICE}|0|alpha"
data_payload="DATA|edge-data|${SENDER_SERVICE}|1|alpha|bogus"

send_udp "$invalid_payload"
send_udp "$ttl_payload"
send_udp "$data_payload"

found_invalid=0
found_ttl=0
found_not_origin=0

deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
  if echo "$logs" | grep -q "req_state=drop_invalid"; then
    found_invalid=1
  fi
  if echo "$logs" | grep -q "req_state=drop_ttl.*id=edge-ttl"; then
    found_ttl=1
  fi
  if echo "$logs" | grep -q "data_state=drop_not_origin.*id=edge-data"; then
    found_not_origin=1
  fi
  if [ $found_invalid -eq 1 ] && [ $found_ttl -eq 1 ] && [ $found_not_origin -eq 1 ]; then
    echo "PASS: observed drop_invalid, drop_ttl, and drop_not_origin behaviors."
    echo "---- edge-case trace ----"
    echo "$logs" | grep -E "req_state=drop_invalid|req_state=drop_ttl.*id=edge-ttl|data_state=drop_not_origin.*id=edge-data" || true
    exit 0
  fi
  sleep 1
done

echo "FAIL: did not observe expected edge-case drops within ${TIMEOUT_SEC}s." >&2
DockerLogs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
echo "$DockerLogs" | grep -E "req_state=|data_state=" || true
exit 1
