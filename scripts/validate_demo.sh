#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this validation script." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this validation script." >&2
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
  if echo "$logs" | grep -q "stored key ${EXPECT_KEY}"; then
    echo "PASS: observed stored key ${EXPECT_KEY} in logs."
    exit 0
  fi
  sleep 1
done

echo "FAIL: did not observe stored key ${EXPECT_KEY} within ${TIMEOUT_SEC}s." >&2
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color || true
exit 1
