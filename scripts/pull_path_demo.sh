#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this pull-path helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this pull-path helper." >&2
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

logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
events="$(echo "$logs" | awk -v key="$EXPECT_KEY" '
  match($0, /^([^[:space:]]+)[[:space:]]+\|[[:space:]]+(.*)$/, m) {
    svc = m[1];
    msg = m[2];
    if (index(msg, "key=" key) > 0) {
      id = "";
      origin = "";
      if (match(msg, /id=[^ ]+/, m2)) {
        id = substr(msg, RSTART + 3, RLENGTH - 3);
      }
      if (match(msg, /origin=[^ ]+/, m3)) {
        origin = substr(msg, RSTART + 7, RLENGTH - 7);
      }
      if (msg ~ /req_state=originated/) {
        print "originated\t" id "\t" origin "\t" svc;
      } else if (msg ~ /req_state=serve_local/) {
        print "serve_local\t" id "\t" origin "\t" svc;
      } else if (msg ~ /data_state=store_local/) {
        print "store_local\t" id "\t" origin "\t" svc;
      }
    }
  }
')"

declare -A origin_by_id=()
declare -A served_by_id=()
declare -A stored_by_id=()
declare -A seen_id=()
declare -a order=()

while IFS=$'\t' read -r event id origin svc; do
  if [ -z "$id" ]; then
    continue
  fi
  case "$event" in
    originated)
      if [ -z "${seen_id[$id]+x}" ]; then
        order+=("$id")
        seen_id[$id]=1
      fi
      if [ -n "$origin" ]; then
        origin_by_id[$id]="$origin"
      fi
      ;;
    serve_local)
      served_by_id[$id]="$svc"
      if [ -n "$origin" ] && [ -z "${origin_by_id[$id]+x}" ]; then
        origin_by_id[$id]="$origin"
      fi
      ;;
    store_local)
      stored_by_id[$id]="$svc"
      if [ -n "$origin" ] && [ -z "${origin_by_id[$id]+x}" ]; then
        origin_by_id[$id]="$origin"
      fi
      ;;
  esac
done <<< "$events"

if [ ${#order[@]} -eq 0 ]; then
  for id in "${!served_by_id[@]}" "${!stored_by_id[@]}"; do
    if [ -n "$id" ] && [ -z "${seen_id[$id]+x}" ]; then
      order+=("$id")
      seen_id[$id]=1
    fi
  done
fi

echo "---- pull path summary (key=${EXPECT_KEY}) ----"
if [ ${#order[@]} -eq 0 ]; then
  echo "No request/data events found for key ${EXPECT_KEY}."
  exit 0
fi

for id in "${order[@]}"; do
  origin="${origin_by_id[$id]:-unknown}"
  served="${served_by_id[$id]:-unknown}"
  stored="${stored_by_id[$id]:-unknown}"
  echo "request_id=${id}"
  echo "origin=${origin} served_by=${served} stored_at=${stored}"
  if [ "$served" = "unknown" ] || [ "$stored" = "unknown" ]; then
    echo "path: incomplete"
  elif [ "$served" = "$origin" ]; then
    echo "path: ${origin} (local cache hit)"
  else
    echo "path: ${origin} -> ${served} -> ${stored}"
  fi
  echo ""
done

echo "Note: node labels reflect docker compose service names."
