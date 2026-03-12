#!/usr/bin/env bash
set -euo pipefail

COMPOSE_FILE="${COMPOSE_FILE:-infra/docker-compose.yml}"
PROJECT_NAME="${COMPOSE_PROJECT_NAME:-netos-demo}"
TIMEOUT_SEC="${TIMEOUT_SEC:-25}"
EXPECT_KEY="${EXPECT_KEY:-alpha}"
TABLE_STATS_FORMAT="${TABLE_STATS_FORMAT:-verbose}"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker is required to run this table-stats helper." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "docker compose v2 is required to run this table-stats helper." >&2
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
  docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color | grep -E "req_state=|data_state=|sync_table=" || true
  exit 1
fi

logs="$(docker compose -f "$COMPOSE_FILE" -p "$PROJECT_NAME" logs --no-color 2>/dev/null || true)"
summary="$(echo "$logs" | awk -v format="$TABLE_STATS_FORMAT" '
  function field_value(msg, field,   pattern) {
    pattern = field "=[^ ]+";
    if (match(msg, pattern)) {
      return substr(msg, RSTART + length(field) + 1, RLENGTH - length(field) - 1);
    }
    return "";
  }
  function sync_value(svc, arr,   value) {
    if (svc in st_seen) {
      value = arr[svc];
      return (value != "" ? value : "n/a");
    }
    return "0";
  }

  match($0, /^([^[:space:]]+)[[:space:]]+\|[[:space:]]+(.*)$/, m) {
    svc = m[1];
    msg = m[2];

    if (!(svc in seen)) {
      order[++order_count] = svc;
      seen[svc] = 1;
    }

    if (msg ~ /query_table_size=/) {
      if (!(svc in qt_seen)) {
        qt_nodes++;
        qt_seen[svc] = 1;
      }
      qt_size[svc] = field_value(msg, "query_table_size");
      qt_attempts[svc] = field_value(msg, "query_table_attempts");
      qt_duplicates[svc] = field_value(msg, "query_table_duplicates");
      qt_pruned[svc] = field_value(msg, "query_table_pruned");
    }

    if (msg ~ /sync_table_updates=/) {
      if (!(svc in st_seen)) {
        st_nodes++;
        st_seen[svc] = 1;
      }
      st_keys[svc] = field_value(msg, "size");
      st_updates[svc] = field_value(msg, "sync_table_updates");
      st_duplicate_hits[svc] = field_value(msg, "sync_table_duplicate_hits");
      st_dest_total[svc] = field_value(msg, "sync_table_destinations_total");
      st_evicted_total[svc] = field_value(msg, "sync_table_evicted_total");
    }
  }

  END {
    if (qt_nodes == 0 && st_nodes == 0) {
      print "No table stats found in logs.";
      exit 0;
    }

    if (format == "compact" || format == "summary" || format == "line") {
      print "---- table stats summary (latest per node) ----";
      for (i = 1; i <= order_count; i++) {
        svc = order[i];
        if ((svc in qt_seen) || (svc in st_seen)) {
          qt_size_val = (svc in qt_seen && qt_size[svc] != "" ? qt_size[svc] : "n/a");
          qt_attempts_val = (svc in qt_seen && qt_attempts[svc] != "" ? qt_attempts[svc] : "n/a");
          qt_duplicates_val = (svc in qt_seen && qt_duplicates[svc] != "" ? qt_duplicates[svc] : "n/a");
          qt_pruned_val = (svc in qt_seen && qt_pruned[svc] != "" ? qt_pruned[svc] : "n/a");
          st_keys_val = sync_value(svc, st_keys);
          st_updates_val = sync_value(svc, st_updates);
          st_duplicate_hits_val = sync_value(svc, st_duplicate_hits);
          st_dest_total_val = sync_value(svc, st_dest_total);
          st_evicted_total_val = sync_value(svc, st_evicted_total);
          printf "%s qt[size=%s attempts=%s duplicates=%s pruned=%s] st[keys=%s updates=%s duplicate_hits=%s destinations=%s evicted=%s]\n",
            svc,
            qt_size_val,
            qt_attempts_val,
            qt_duplicates_val,
            qt_pruned_val,
            st_keys_val,
            st_updates_val,
            st_duplicate_hits_val,
            st_dest_total_val,
            st_evicted_total_val;
        }
      }
      exit 0;
    }

    if (qt_nodes > 0) {
      print "---- query table stats (latest per node) ----";
      for (i = 1; i <= order_count; i++) {
        svc = order[i];
        if (svc in qt_seen) {
          printf "%s query_table_size=%s query_table_attempts=%s query_table_duplicates=%s query_table_pruned=%s\n",
            svc,
            (qt_size[svc] != "" ? qt_size[svc] : "n/a"),
            (qt_attempts[svc] != "" ? qt_attempts[svc] : "n/a"),
            (qt_duplicates[svc] != "" ? qt_duplicates[svc] : "n/a"),
            (qt_pruned[svc] != "" ? qt_pruned[svc] : "n/a");
        }
      }
    } else {
      print "No QueryTable stats found in logs.";
    }

    if (st_nodes > 0) {
      print "---- sync table stats (latest per node) ----";
      for (i = 1; i <= order_count; i++) {
        svc = order[i];
        if (svc in st_seen) {
          printf "%s sync_table_keys=%s sync_table_updates=%s sync_table_duplicate_hits=%s sync_table_destinations_total=%s sync_table_evicted_total=%s\n",
            svc,
            (st_keys[svc] != "" ? st_keys[svc] : "n/a"),
            (st_updates[svc] != "" ? st_updates[svc] : "n/a"),
            (st_duplicate_hits[svc] != "" ? st_duplicate_hits[svc] : "n/a"),
            (st_dest_total[svc] != "" ? st_dest_total[svc] : "n/a"),
            (st_evicted_total[svc] != "" ? st_evicted_total[svc] : "n/a");
        }
      }
    } else {
      print "No SyncTable stats found in logs.";
    }

    print "---- table stats summary (concise) ----";
    for (i = 1; i <= order_count; i++) {
      svc = order[i];
      if ((svc in qt_seen) || (svc in st_seen)) {
        qt_size_val = (svc in qt_seen && qt_size[svc] != "" ? qt_size[svc] : "n/a");
        qt_duplicates_val = (svc in qt_seen && qt_duplicates[svc] != "" ? qt_duplicates[svc] : "n/a");
        qt_pruned_val = (svc in qt_seen && qt_pruned[svc] != "" ? qt_pruned[svc] : "n/a");
        st_keys_val = sync_value(svc, st_keys);
        st_updates_val = sync_value(svc, st_updates);
        st_evicted_total_val = sync_value(svc, st_evicted_total);
        printf "%s QueryTable: size=%s dup=%s pruned=%s; SyncTable: keys=%s updates=%s evicted=%s\n",
          svc,
          qt_size_val,
          qt_duplicates_val,
          qt_pruned_val,
          st_keys_val,
          st_updates_val,
          st_evicted_total_val;
      }
    }
  }
')"

echo "$summary"
