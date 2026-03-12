#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

input=""
if [ -t 0 ]; then
  input="$("$ROOT_DIR/scripts/table_stats_demo.sh")"
else
  input="$(cat)"
fi

echo "$input" | awk '
  function is_number(value) {
    return value ~ /^[0-9]+$/
  }
  function qt_status(dup, pruned) {
    if (!is_number(dup) || !is_number(pruned)) {
      return "unknown"
    }
    if ((dup + 0) > 0 || (pruned + 0) > 0) {
      return "attention"
    }
    return "healthy"
  }
  function st_status(evicted) {
    if (!is_number(evicted)) {
      return "unknown"
    }
    if ((evicted + 0) > 0) {
      return "attention"
    }
    return "healthy"
  }
  function emit(svc, dup, pruned, evicted) {
    printf "%s QueryTable %s: dup=%s, pruned=%s; SyncTable %s: evicted=%s\n",
      svc,
      qt_status(dup, pruned),
      dup,
      pruned,
      st_status(evicted),
      evicted
  }

  match($0, /^([^[:space:]]+)[[:space:]]+QueryTable: size=[^ ]+ dup=([^ ]+) pruned=([^;]+); SyncTable: keys=[^ ]+ updates=[^ ]+ evicted=([^ ]+)/, m) {
    emit(m[1], m[2], m[3], m[4])
    found = 1
    next
  }

  match($0, /^([^[:space:]]+)[[:space:]]+qt\[[^]]*duplicates=([^ ]+)[^]]*pruned=([^ ]+)\][[:space:]]+st\[[^]]*evicted=([^ ]+)\]/, m) {
    emit(m[1], m[2], m[3], m[4])
    found = 1
    next
  }

  END {
    if (!found) {
      print "No table stats summary lines found to summarize."
    }
  }
'
