#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"
REQUEST_TTL="${REQUEST_TTL:-3}"
TARGET_SERVICE="${TARGET_SERVICE:-node1}"
TARGET_PORT="${TARGET_PORT:-9001}"
SENDER_SERVICE="${SENDER_SERVICE:-node2}"
REQUEST_ID="${REQUEST_ID:-dup-$(date +%s)-$$-$RANDOM}"
REQUEST_ORIGIN="${REQUEST_ORIGIN:-$SENDER_SERVICE}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this duplicate-request helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this duplicate-request helper." >&2
  exit 1
fi

echo "duplicate-request: starting NetOS demo via docker compose..."
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" up --build -d

cleanup() {
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" down -v >/dev/null 2>&1 || true
}
trap cleanup EXIT

wait_deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $wait_deadline ]; do
  logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
  if echo "$logs" | grep -q "listening on"; then
    break
  fi
  sleep 1
done

if [ $SECONDS -ge $wait_deadline ]; then
  echo "FAIL [duplicate-request]: nodes did not reach listening state within ${TIMEOUT_SEC}s." >&2
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color || true
  exit 1
fi

send_udp() {
  local payload="$1"
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$SENDER_SERVICE" \
    env PAYLOAD="$payload" TARGET_HOST="$TARGET_SERVICE" TARGET_PORT="$TARGET_PORT" \
    bash -lc 'printf "%s" "$PAYLOAD" > /dev/udp/${TARGET_HOST}/${TARGET_PORT}'
}

payload="REQ|${REQUEST_ID}|${REQUEST_ORIGIN}|${REQUEST_TTL}|${EXPECT_KEY}"

echo "duplicate-request: sending duplicate requests id=${REQUEST_ID} origin=${REQUEST_ORIGIN} key=${EXPECT_KEY}"
send_udp "$payload"
sleep 0.2
send_udp "$payload"

deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
  if echo "$logs" | grep -F "req_state=drop_duplicate" | grep -F "id=${REQUEST_ID}" >/dev/null; then
    echo "PASS [duplicate-request]: observed req_state=drop_duplicate for request_id ${REQUEST_ID}."
    echo "---- duplicate-request trace ----"
    echo "$logs" | grep -F "id=${REQUEST_ID}" | grep -E "req_state=|data_state=" || true
    exit 0
  fi
  sleep 1
done

echo "FAIL [duplicate-request]: did not observe req_state=drop_duplicate for request_id ${REQUEST_ID} within ${TIMEOUT_SEC}s." >&2
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -F "id=${REQUEST_ID}" || true
exit 1
