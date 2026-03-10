#!/usr/bin/env bash
set -euo pipefail

DEFAULT_DIRS=("infra/topology/2-node" "infra/topology/3-node")

if [ "$#" -gt 0 ]; then
  TOPOLOGY_DIRS=("$@")
else
  TOPOLOGY_DIRS=("${DEFAULT_DIRS[@]}")
fi

status=0

fail() {
  echo "ERROR: $*" >&2
  status=1
}

warn() {
  echo "WARN: $*" >&2
}

trim() {
  local s="$1"
  s="${s#"${s%%[![:space:]]*}"}"
  s="${s%"${s##*[![:space:]]}"}"
  printf '%s' "$s"
}

strip_quotes() {
  local s="$1"
  if [[ $s == "\""*"\"" ]] && [ ${#s} -ge 2 ]; then
    s="${s:1:${#s}-2}"
  elif [[ $s == "'"*"'" ]] && [ ${#s} -ge 2 ]; then
    s="${s:1:${#s}-2}"
  fi
  printf '%s' "$s"
}

get_env_value() {
  local key="$1"
  local file="$2"
  local line
  line="$(grep -E "^(export[[:space:]]+)?${key}=" "$file" | tail -n 1 || true)"
  if [ -z "$line" ]; then
    printf '%s' ""
    return
  fi
  line="${line#export }"
  line="${line#${key}=}"
  line="$(strip_quotes "$line")"
  printf '%s' "$(trim "$line")"
}

is_int() {
  [[ $1 =~ ^[0-9]+$ ]]
}

in_range_port() {
  local port="$1"
  if ! is_int "$port"; then
    return 1
  fi
  if [ "$port" -lt 1 ] || [ "$port" -gt 65535 ]; then
    return 1
  fi
  return 0
}

check_dir() {
  local dir="$1"
  local -A node_id_to_file=()
  local -A node_id_to_port=()
  local -A port_to_file=()

  if [ ! -d "$dir" ]; then
    fail "topology dir not found: $dir"
    return
  fi

  shopt -s nullglob
  local env_files=("$dir"/*.env)
  shopt -u nullglob

  if [ ${#env_files[@]} -eq 0 ]; then
    fail "no .env files found in $dir"
    return
  fi

  echo "- $dir (${#env_files[@]} files)"

  for file in "${env_files[@]}"; do
    local node_id
    local bind_port
    local neighbors
    node_id="$(get_env_value NETOS_NODE_ID "$file")"
    bind_port="$(get_env_value NETOS_BIND_PORT "$file")"
    neighbors="$(get_env_value NETOS_NEIGHBORS "$file")"

    if [ -z "$node_id" ]; then
      fail "$file: NETOS_NODE_ID missing"
    else
      if [ -n "${node_id_to_file[$node_id]:-}" ] && [ "${node_id_to_file[$node_id]}" != "$file" ]; then
        fail "$file: NETOS_NODE_ID '$node_id' duplicates ${node_id_to_file[$node_id]}"
      fi
      node_id_to_file[$node_id]="$file"
    fi

    if [ -z "$bind_port" ]; then
      fail "$file: NETOS_BIND_PORT missing"
    elif ! in_range_port "$bind_port"; then
      fail "$file: NETOS_BIND_PORT '$bind_port' is not a valid port"
    else
      if [ -n "${node_id_to_port[$node_id]:-}" ] && [ "${node_id_to_port[$node_id]}" != "$bind_port" ]; then
        fail "$file: NETOS_BIND_PORT '$bind_port' conflicts with ${node_id_to_port[$node_id]} for node $node_id"
      fi
      if [ -n "${port_to_file[$bind_port]:-}" ]; then
        fail "$file: NETOS_BIND_PORT '$bind_port' duplicates ${port_to_file[$bind_port]}"
      fi
      node_id_to_port[$node_id]="$bind_port"
      port_to_file[$bind_port]="$file"
    fi

    if [ -z "$neighbors" ]; then
      fail "$file: NETOS_NEIGHBORS missing"
    fi
  done

  for file in "${env_files[@]}"; do
    local neighbors_raw
    neighbors_raw="$(get_env_value NETOS_NEIGHBORS "$file")"
    if [ -z "$neighbors_raw" ]; then
      continue
    fi
    IFS=',' read -r -a neighbors <<< "$neighbors_raw"
    for entry in "${neighbors[@]}"; do
      entry="$(trim "$entry")"
      if [ -z "$entry" ]; then
        fail "$file: empty neighbor entry in NETOS_NEIGHBORS"
        continue
      fi
      if [[ "$entry" != *:* ]]; then
        fail "$file: neighbor '$entry' must be host:port"
        continue
      fi
      local host
      local port
      host="${entry%%:*}"
      port="${entry##*:}"
      host="$(trim "$host")"
      port="$(trim "$port")"
      if [ -z "$host" ] || [ -z "$port" ]; then
        fail "$file: neighbor '$entry' must be host:port"
        continue
      fi
      if ! in_range_port "$port"; then
        fail "$file: neighbor '$entry' has invalid port"
        continue
      fi
      if [ -n "${node_id_to_port[$host]:-}" ]; then
        local expected_port
        expected_port="${node_id_to_port[$host]}"
        if [ "$port" != "$expected_port" ]; then
          fail "$file: neighbor '$host' uses port $port but $host binds $expected_port"
        fi
      else
        warn "$file: neighbor host '$host' not found in topology nodes"
      fi
    done
  done
}

echo "Checking topology env files..."
for dir in "${TOPOLOGY_DIRS[@]}"; do
  check_dir "$dir"
done

if [ "$status" -eq 0 ]; then
  echo "PASS: topology env files look consistent."
  exit 0
fi

echo "FAIL: topology env file checks failed." >&2
exit 1
