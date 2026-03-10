#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.3-node.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo-3node}"
TIMEOUT_SEC="${TIMEOUT_SEC:-35}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"
FINAL_NODE="${FINAL_NODE:-node3}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this hop-story helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this hop-story helper." >&2
  exit 1
fi

echo "Starting NetOS 3-node demo via docker compose..."
docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" up --build -d

cleanup() {
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" down -v >/dev/null 2>&1 || true
}
trap cleanup EXIT

deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
  node_logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color "$FINAL_NODE" 2>/dev/null || true)"
  if echo "$node_logs" | grep -q "data_state=store_local.*key=${EXPECT_KEY}"; then
    echo "PASS: observed ${FINAL_NODE} data_state=store_local for key ${EXPECT_KEY}."
    break
  fi
  sleep 1
done

if [ $SECONDS -ge $deadline ]; then
  echo "FAIL: did not observe ${FINAL_NODE} data_state=store_local for key ${EXPECT_KEY} within ${TIMEOUT_SEC}s." >&2
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -E "req_state=|data_state=" || true
  exit 1
fi

logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
events="$(echo "$logs" | awk -v key="$EXPECT_KEY" '
  match($0, /^([^[:space:]]+)[[:space:]]+\|[[:space:]]+(.*)$/, m) {
    svc = m[1];
    msg = m[2];
    if (msg ~ ("seeded key " key)) {
      ts="";
      if (match(msg, /^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}/)) {
        ts = substr(msg, RSTART, RLENGTH);
      }
      print ts "\t" svc "\tseeded\t\t\t" msg;
      next;
    }
    if (index(msg, "key=" key) > 0) {
      ts="";
      if (match(msg, /^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}/)) {
        ts = substr(msg, RSTART, RLENGTH);
      }
      id="";
      origin="";
      if (match(msg, /id=[^ ]+/, m2)) {
        id = substr(msg, RSTART + 3, RLENGTH - 3);
      }
      if (match(msg, /origin=[^ ]+/, m3)) {
        origin = substr(msg, RSTART + 7, RLENGTH - 7);
      }
      if (msg ~ /req_state=originated/) {
        print ts "\t" svc "\toriginated\t" id "\t" origin "\t" msg;
      } else if (msg ~ /req_state=serve_local/) {
        print ts "\t" svc "\tserve_local\t" id "\t" origin "\t" msg;
      } else if (msg ~ /data_state=store_local/) {
        print ts "\t" svc "\tstore_local\t" id "\t" origin "\t" msg;
      }
    }
  }
')"

echo "---- hop-through story (key=${EXPECT_KEY}) ----"
if [ -z "$events" ]; then
  echo "No request/data events found for key ${EXPECT_KEY}."
  exit 0
fi

declare -A origin_by_id=()
declare -A served_by_id=()
declare -A stored_by_id=()
declare -A served_by_origin=()
declare -A stored_by_origin=()
seeded_by=""

step=0
while IFS=$'\t' read -r ts svc event id origin msg; do
  if [ -z "$event" ]; then
    continue
  fi
  step=$((step + 1))
  label=""
  if [ -z "$origin" ] && { [ "$event" = "originated" ] || [ "$event" = "store_local" ]; }; then
    origin="$svc"
  fi
  case "$event" in
    seeded)
      label="seeded key"
      if [ -z "$seeded_by" ]; then
        seeded_by="$svc"
      fi
      ;;
    originated)
      label="originated request"
      if [ -n "$id" ]; then
        origin_by_id[$id]="$origin"
      fi
      ;;
    serve_local)
      label="served from cache"
      if [ -n "$id" ]; then
        served_by_id[$id]="$svc"
        if [ -n "$origin" ] && [ -z "${origin_by_id[$id]+x}" ]; then
          origin_by_id[$id]="$origin"
        fi
        if [ -n "$origin" ]; then
          served_by_origin[$origin]="$svc"
        fi
      fi
      ;;
    store_local)
      label="stored data"
      if [ -n "$id" ]; then
        stored_by_id[$id]="$svc"
        if [ -n "$origin" ] && [ -z "${origin_by_id[$id]+x}" ]; then
          origin_by_id[$id]="$origin"
        fi
        if [ -n "$origin" ]; then
          stored_by_origin[$origin]="$svc"
        fi
      fi
      ;;
    *)
      label="$event"
      ;;
  esac

  ts_display="$ts"
  if [ -z "$ts_display" ]; then
    ts_display="unknown_time"
  fi

  detail=""
  if [ -n "$id" ]; then
    detail="id=${id}"
  fi
  if [ -n "$origin" ]; then
    if [ -n "$detail" ]; then
      detail+=" "
    fi
    detail+="origin=${origin}"
  fi

  printf "%d. %s %s %s" "$step" "$ts_display" "$svc" "$label"
  if [ -n "$detail" ]; then
    printf " %s" "$detail"
  fi
  printf "\n"

done <<< "$events"

find_request_id_for_node() {
  local target="$1"
  local id
  for id in "${!origin_by_id[@]}"; do
    if [ "${origin_by_id[$id]}" = "$target" ]; then
      echo "$id"
      return 0
    fi
  done
  for id in "${!stored_by_id[@]}"; do
    if [ "${stored_by_id[$id]}" = "$target" ]; then
      echo "$id"
      return 0
    fi
  done
  return 1
}

resolve_served_by() {
  local id="$1"
  local origin="$2"
  local fallback="$3"
  local served="${served_by_id[$id]:-}"
  if [ -z "$served" ] && [ -n "$origin" ]; then
    served="${served_by_origin[$origin]:-}"
  fi
  if [ -z "$served" ] && [ -n "$fallback" ]; then
    served="$fallback"
  fi
  if [ -z "$served" ]; then
    served="unknown"
  fi
  echo "$served"
}

resolve_stored_by() {
  local id="$1"
  local origin="$2"
  local stored="${stored_by_id[$id]:-}"
  if [ -z "$stored" ] && [ -n "$origin" ]; then
    stored="${stored_by_origin[$origin]:-}"
  fi
  if [ -z "$stored" ] && [ -n "$origin" ]; then
    stored="$origin"
  fi
  if [ -z "$stored" ]; then
    stored="unknown"
  fi
  echo "$stored"
}

final_id="$(find_request_id_for_node "$FINAL_NODE" || true)"
intermediate=""
upstream=""
intermediate_id=""

if [ -n "$final_id" ]; then
  intermediate="${served_by_id[$final_id]:-}"
fi
if [ -n "$intermediate" ]; then
  intermediate_id="$(find_request_id_for_node "$intermediate" || true)"
  if [ -n "$intermediate_id" ]; then
    upstream="${served_by_id[$intermediate_id]:-}"
  fi
fi

echo "---- story beats ----"
story_step=0
if [ -n "$seeded_by" ]; then
  story_step=$((story_step + 1))
  echo "${story_step}. ${seeded_by} seeded ${EXPECT_KEY}."
fi

if [ -n "$intermediate" ]; then
  story_step=$((story_step + 1))
  intermediate_served="$(resolve_served_by "$intermediate_id" "$intermediate" "$seeded_by")"
  intermediate_stored="$(resolve_stored_by "$intermediate_id" "$intermediate")"
  if [ -n "$intermediate_id" ]; then
    echo "${story_step}. ${intermediate} requested ${EXPECT_KEY} (id=${intermediate_id}). served by ${intermediate_served}, stored at ${intermediate_stored}."
  else
    echo "${story_step}. ${intermediate} requested ${EXPECT_KEY}. served by ${intermediate_served}, stored at ${intermediate_stored}."
  fi
fi

if [ -n "$final_id" ]; then
  story_step=$((story_step + 1))
  final_served="$(resolve_served_by "$final_id" "$FINAL_NODE" "")"
  final_stored="$(resolve_stored_by "$final_id" "$FINAL_NODE")"
  echo "${story_step}. ${FINAL_NODE} requested ${EXPECT_KEY} (id=${final_id}). served by ${final_served}, stored at ${final_stored}."
fi

if [ $story_step -eq 0 ]; then
  echo "No story beats found for key ${EXPECT_KEY}."
fi

echo "---- hop summary ----"
if [ -n "$intermediate_id" ]; then
  summary_origin="${origin_by_id[$intermediate_id]:-${intermediate}}"
  summary_served="$(resolve_served_by "$intermediate_id" "$summary_origin" "$seeded_by")"
  summary_stored="$(resolve_stored_by "$intermediate_id" "$summary_origin")"
  echo "request_id=${intermediate_id} origin=${summary_origin:-unknown} served_by=${summary_served} stored_at=${summary_stored}"
fi
if [ -n "$final_id" ]; then
  summary_origin="${origin_by_id[$final_id]:-${FINAL_NODE}}"
  summary_served="$(resolve_served_by "$final_id" "$summary_origin" "")"
  summary_stored="$(resolve_stored_by "$final_id" "$summary_origin")"
  echo "request_id=${final_id} origin=${summary_origin:-unknown} served_by=${summary_served} stored_at=${summary_stored}"
fi

if [ -n "$upstream" ] && [ -n "$intermediate" ]; then
  echo "hop_chain: ${upstream} -> ${intermediate} -> ${FINAL_NODE}"
elif [ -n "$intermediate" ]; then
  echo "hop_chain: ${intermediate} -> ${FINAL_NODE}"
elif [ -n "$seeded_by" ]; then
  echo "seeded_by: ${seeded_by}"
fi

echo "Note: node labels reflect docker compose service names."
