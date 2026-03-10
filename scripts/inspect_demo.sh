#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"
INSPECT_KEYS="${INSPECT_KEYS:-$EXPECT_KEY}"
REDIS_SOCKET="${REDIS_SOCKET:-/var/run/redis/redis.sock}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this inspection helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this inspection helper." >&2
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
  if echo "$logs" | grep -q "data_state=store_local.*key=${EXPECT_KEY}"; then
    echo "PASS: observed data_state=store_local for key ${EXPECT_KEY}."
    break
  fi
  sleep 1
done

if [ $SECONDS -ge $deadline ]; then
  echo "FAIL: did not observe data_state=store_local for key ${EXPECT_KEY} within ${TIMEOUT_SEC}s." >&2
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -E "req_state=|data_state=" || true
  exit 1
fi

services="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" config --services 2>/dev/null || true)"
if [ -z "$services" ]; then
  echo "No services found for compose file ${COMPOSE_FILE}." >&2
  exit 1
fi

IFS=',' read -r -a inspect_keys <<< "$INSPECT_KEYS"

for service in $services; do
  echo "---- ${service} ----"
  config_line="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color "$service" 2>/dev/null | grep "config node_id=" | tail -n 1 || true)"
  if [ -n "$config_line" ]; then
    echo "$config_line"
  else
    echo "config: not found in logs"
  fi

  dbsize="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$service" redis-cli -s "$REDIS_SOCKET" --raw DBSIZE 2>/dev/null || true)"
  if [ -n "$dbsize" ]; then
    echo "redis dbsize=${dbsize}"
  else
    echo "redis dbsize=unavailable"
  fi

  keys_all="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$service" redis-cli -s "$REDIS_SOCKET" --raw KEYS '*' 2>/dev/null || true)"
  if [ -n "$keys_all" ]; then
    echo "redis keys:"
    echo "$keys_all"
  else
    echo "redis keys: <none>"
  fi

  for key in "${inspect_keys[@]}"; do
    key_trimmed="$(echo "$key" | xargs)"
    if [ -z "$key_trimmed" ]; then
      continue
    fi
    exists="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$service" redis-cli -s "$REDIS_SOCKET" --raw EXISTS "$key_trimmed" 2>/dev/null || true)"
    if [ "$exists" = "1" ]; then
      value="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" exec -T "$service" redis-cli -s "$REDIS_SOCKET" --raw GET "$key_trimmed" 2>/dev/null || true)"
      echo "key ${key_trimmed}=${value}"
    else
      echo "key ${key_trimmed}=<missing>"
    fi
  done

done
