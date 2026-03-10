#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.3-node.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo-3node}"
TIMEOUT_SEC="${TIMEOUT_SEC:-35}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this validation script." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this validation script." >&2
  exit 1
fi

echo "Starting NetOS 3-node demo via docker compose..."
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" up --build -d

cleanup() {
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" down -v >/dev/null 2>&1 || true
}
trap cleanup EXIT

found_node1=0
found_node2_store=0
found_node2_serve=0
found_node3_store=0

deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  node1_logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color node1 2>/dev/null || true)"
  if echo "$node1_logs" | grep -q "req_state=serve_local.*key=${EXPECT_KEY}.*origin=node2"; then
    found_node1=1
  fi

  node2_logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color node2 2>/dev/null || true)"
  if echo "$node2_logs" | grep -q "data_state=store_local.*key=${EXPECT_KEY}"; then
    found_node2_store=1
  fi
  if echo "$node2_logs" | grep -q "req_state=serve_local.*key=${EXPECT_KEY}.*origin=node3"; then
    found_node2_serve=1
  fi

  node3_logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color node3 2>/dev/null || true)"
  if echo "$node3_logs" | grep -q "data_state=store_local.*key=${EXPECT_KEY}"; then
    found_node3_store=1
  fi

  if [ $found_node1 -eq 1 ] && [ $found_node2_store -eq 1 ] && [ $found_node2_serve -eq 1 ] && [ $found_node3_store -eq 1 ]; then
    echo "PASS: observed hop-through for key ${EXPECT_KEY} (node1 -> node2 -> node3)."
    exit 0
  fi
  sleep 1
done

echo "FAIL: did not observe hop-through for key ${EXPECT_KEY} within ${TIMEOUT_SEC}s." >&2
if [ $found_node1 -eq 0 ]; then
  echo "missing: node1 req_state=serve_local origin=node2" >&2
fi
if [ $found_node2_store -eq 0 ]; then
  echo "missing: node2 data_state=store_local" >&2
fi
if [ $found_node2_serve -eq 0 ]; then
  echo "missing: node2 req_state=serve_local origin=node3" >&2
fi
if [ $found_node3_store -eq 0 ]; then
  echo "missing: node3 data_state=store_local" >&2
fi

docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -E "req_state=|data_state=" || true
exit 1
