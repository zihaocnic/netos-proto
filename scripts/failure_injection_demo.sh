#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-failure}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"
TARGET_SERVICE="${TARGET_SERVICE:-node1}"
TARGET_PORT="${TARGET_PORT:-9001}"
SENDER_SERVICE="${SENDER_SERVICE:-node2}"
FAILURE_MODE="${FAILURE_MODE:-both}"
REQUEST_TTL="${REQUEST_TTL:-0}"
DATA_TTL="${DATA_TTL:-1}"
REQUEST_ID="${REQUEST_ID:-fail-ttl-$(date +%s)-$$-$RANDOM}"
DATA_ID="${DATA_ID:-fail-data-$(date +%s)-$$-$RANDOM}"

case "$FAILURE_MODE" in
  ttl|not-origin|both)
    ;;
  *)
    echo "FAILURE_MODE must be ttl, not-origin, or both (got '$FAILURE_MODE')." >&2
    exit 1
    ;;
 esac

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this failure-injection helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this failure-injection helper." >&2
  exit 1
fi

echo "failure-injection: starting NetOS demo via docker compose..."
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
  echo "FAIL [failure-injection]: nodes did not reach listening state within ${TIMEOUT_SEC}s." >&2
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color || true
  exit 1
fi

send_udp() {
  local payload="$1"
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$SENDER_SERVICE" \
    env PAYLOAD="$payload" TARGET_HOST="$TARGET_SERVICE" TARGET_PORT="$TARGET_PORT" \
    bash -lc 'printf "%s" "$PAYLOAD" > /dev/udp/${TARGET_HOST}/${TARGET_PORT}'
}

if [ "$FAILURE_MODE" = "ttl" ] || [ "$FAILURE_MODE" = "both" ]; then
  ttl_payload="REQ|${REQUEST_ID}|${SENDER_SERVICE}|${REQUEST_TTL}|${EXPECT_KEY}"
  echo "failure-injection: sending TTL-expired request id=${REQUEST_ID} ttl=${REQUEST_TTL}"
  send_udp "$ttl_payload"
fi

if [ "$FAILURE_MODE" = "not-origin" ] || [ "$FAILURE_MODE" = "both" ]; then
  data_payload="DATA|${DATA_ID}|${SENDER_SERVICE}|${DATA_TTL}|${EXPECT_KEY}|bogus"
  echo "failure-injection: sending not-origin data id=${DATA_ID} origin=${SENDER_SERVICE}"
  send_udp "$data_payload"
fi

found_ttl=0
found_not_origin=0

deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
  if [ "$FAILURE_MODE" = "ttl" ] || [ "$FAILURE_MODE" = "both" ]; then
    if echo "$logs" | grep -F "req_state=drop_ttl" | grep -F "id=${REQUEST_ID}" >/dev/null; then
      found_ttl=1
    fi
  fi
  if [ "$FAILURE_MODE" = "not-origin" ] || [ "$FAILURE_MODE" = "both" ]; then
    if echo "$logs" | grep -F "data_state=drop_not_origin" | grep -F "id=${DATA_ID}" >/dev/null; then
      found_not_origin=1
    fi
  fi

  if [ "$FAILURE_MODE" = "ttl" ] && [ $found_ttl -eq 1 ]; then
    break
  fi
  if [ "$FAILURE_MODE" = "not-origin" ] && [ $found_not_origin -eq 1 ]; then
    break
  fi
  if [ "$FAILURE_MODE" = "both" ] && [ $found_ttl -eq 1 ] && [ $found_not_origin -eq 1 ]; then
    break
  fi
  sleep 1
done

if [ "$FAILURE_MODE" = "ttl" ] && [ $found_ttl -eq 1 ]; then
  echo "PASS [failure-injection]: observed req_state=drop_ttl for request_id ${REQUEST_ID}."
elif [ "$FAILURE_MODE" = "not-origin" ] && [ $found_not_origin -eq 1 ]; then
  echo "PASS [failure-injection]: observed data_state=drop_not_origin for request_id ${DATA_ID}."
elif [ "$FAILURE_MODE" = "both" ] && [ $found_ttl -eq 1 ] && [ $found_not_origin -eq 1 ]; then
  echo "PASS [failure-injection]: observed drop_ttl and drop_not_origin events."
else
  echo "FAIL [failure-injection]: did not observe expected drop events within ${TIMEOUT_SEC}s." >&2
  if [ "$FAILURE_MODE" = "ttl" ] || [ "$FAILURE_MODE" = "both" ]; then
    echo "- missing req_state=drop_ttl for id=${REQUEST_ID}" >&2
  fi
  if [ "$FAILURE_MODE" = "not-origin" ] || [ "$FAILURE_MODE" = "both" ]; then
    echo "- missing data_state=drop_not_origin for id=${DATA_ID}" >&2
  fi
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -E "req_state=|data_state=" || true
  exit 1
fi

echo "---- failure-injection trace ----"
echo "$logs" | grep -E "req_state=drop_ttl|data_state=drop_not_origin" | grep -E "id=${REQUEST_ID}|id=${DATA_ID}" || true
