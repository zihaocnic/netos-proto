#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this trace helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this trace helper." >&2
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
    echo "---- trace (key=${EXPECT_KEY}) ----"
    echo "$logs" | grep -E "req_state=|data_state=" | grep "key=${EXPECT_KEY}" || true
    echo "---- timeline (key=${EXPECT_KEY}) ----"
    timeline="$(echo "$logs" | awk -v key="$EXPECT_KEY" '
      BEGIN { step = 0 }
      match($0, /^([^[:space:]]+)[[:space:]]+\|[[:space:]]+(.*)$/, m) {
        svc = m[1];
        msg = m[2];
        if (index(msg, "key=" key) == 0) {
          next;
        }
        if (msg !~ /req_state=|data_state=/) {
          next;
        }
        ts = "unknown_time";
        if (match(msg, /^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}/)) {
          ts = substr(msg, RSTART, RLENGTH);
        }
        kind = "";
        state = "";
        if (match(msg, /req_state=[^ ]+/)) {
          kind = "req";
          state = substr(msg, RSTART + 10, RLENGTH - 10);
        } else if (match(msg, /data_state=[^ ]+/)) {
          kind = "data";
          state = substr(msg, RSTART + 11, RLENGTH - 11);
        }
        id = "";
        origin = "";
        from = "";
        if (match(msg, /id=[^ ]+/)) {
          id = substr(msg, RSTART + 3, RLENGTH - 3);
        }
        if (match(msg, /origin=[^ ]+/)) {
          origin = substr(msg, RSTART + 7, RLENGTH - 7);
        }
        if (match(msg, /from=[^ ]+/)) {
          from = substr(msg, RSTART + 5, RLENGTH - 5);
        }
        step++;
        printf "%d. %s %s %s:%s", step, ts, svc, kind, state;
        if (id != "") {
          printf " id=%s", id;
        }
        if (origin != "") {
          printf " origin=%s", origin;
        }
        if (from != "") {
          printf " from=%s", from;
        }
        printf "\n";
      }
    ')"
    if [ -n "$timeline" ]; then
      echo "$timeline"
    else
      echo "No request/data events found for key ${EXPECT_KEY}."
    fi
    exit 0
  fi
  sleep 1
done

echo "FAIL: did not observe data_state=store_local for key ${EXPECT_KEY} within ${TIMEOUT_SEC}s." >&2
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -E "req_state=|data_state=" || true
exit 1
