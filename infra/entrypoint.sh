#!/usr/bin/env bash
set -euo pipefail

SOCKET_PATH="${NETOS_REDIS_SOCKET:-/var/run/redis/redis.sock}"
SOCKET_DIR="$(dirname "$SOCKET_PATH")"

mkdir -p "$SOCKET_DIR"

redis-server --save "" --appendonly no --port 0 --unixsocket "$SOCKET_PATH" --unixsocketperm 770 --dir /tmp &
REDIS_PID=$!

for _ in $(seq 1 30); do
  if [ -S "$SOCKET_PATH" ]; then
    break
  fi
  sleep 0.1
done

exec /opt/netos/bin/netos-node
