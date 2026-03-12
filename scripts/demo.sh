#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
  cat <<'USAGE'
NetOS demo driver (Phase-1)

Usage:
  ./scripts/demo.sh <command> [options]

Commands:
  start          Start docker compose (foreground by default)
  validate       Run validation helper
  trace          Run trace helper
  table-stats    Run table stats summary helper
  table-health   Run table stats health helper
  pull-path      Run pull path helper
  hop-story      Run hop-through story helper (3-node)
  inspect        Run inspection helper
  edge-cases     Run edge-case helper
  check-topology Validate topology env files
  help           Show this help

Common options:
  --2-node            Use the 2-node topology (default)
  --3-node            Use the 3-node topology
  --compose FILE      Override compose file
  --project NAME      Override compose project name
  --timeout SECONDS   Override TIMEOUT_SEC
  --key KEY           Override EXPECT_KEY

Command options:
  start: --detach
  table-stats: --table-format FORMAT
  hop-story: --final-node NODE
  inspect: --inspect-keys k1,k2
  edge-cases: --target-service SVC --target-port PORT --sender-service SVC
  check-topology: [dir...]

Examples:
  ./scripts/demo.sh start
  ./scripts/demo.sh start --3-node
  ./scripts/demo.sh validate
  ./scripts/demo.sh trace --3-node
  ./scripts/demo.sh table-stats
  ./scripts/demo.sh table-stats --table-format compact
  ./scripts/demo.sh table-health
  ./scripts/demo.sh pull-path --3-node
  ./scripts/demo.sh hop-story --3-node
  ./scripts/demo.sh inspect --inspect-keys alpha,beta
  ./scripts/demo.sh edge-cases
  ./scripts/demo.sh check-topology
USAGE
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

command="$1"
shift

case "$command" in
  help|-h|--help)
    usage
    exit 0
    ;;
  start|validate|trace|table-stats|table-health|pull-path|hop-story|inspect|edge-cases|check-topology)
    ;;
  *)
    echo "Unknown command: $command" >&2
    usage
    exit 1
    ;;
esac

topology="2"
compose_file=""
project_name=""
timeout=""
expect_key=""
final_node=""
inspect_keys=""
target_service=""
target_port=""
sender_service=""
detach=0
table_stats_format=""
extra_topology_dirs=()

while [ "$#" -gt 0 ]; do
  case "$1" in
    --2-node|--2)
      topology="2"
      shift
      ;;
    --3-node|--3)
      topology="3"
      shift
      ;;
    --compose)
      compose_file="$2"
      shift 2
      ;;
    --project)
      project_name="$2"
      shift 2
      ;;
    --timeout)
      timeout="$2"
      shift 2
      ;;
    --key)
      expect_key="$2"
      shift 2
      ;;
    --final-node)
      final_node="$2"
      shift 2
      ;;
    --inspect-keys)
      inspect_keys="$2"
      shift 2
      ;;
    --target-service)
      target_service="$2"
      shift 2
      ;;
    --target-port)
      target_port="$2"
      shift 2
      ;;
    --sender-service)
      sender_service="$2"
      shift 2
      ;;
    --detach|-d)
      detach=1
      shift
      ;;
    --table-format|--table-stats-format)
      table_stats_format="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      if [ "$command" = "check-topology" ]; then
        extra_topology_dirs+=("$1")
        shift
      else
        echo "Unknown option: $1" >&2
        usage
        exit 1
      fi
      ;;
  esac
done

if [ "$command" = "hop-story" ] && [ -z "$compose_file" ] && [ "$topology" != "3" ] && [ -z "${COMPOSE_FILE:-}" ]; then
  echo "hop-story is intended for the 3-node demo. Use --3-node or --compose." >&2
  exit 1
fi

default_compose="infra/docker-compose.yml"
if [ "$topology" = "3" ]; then
  default_compose="infra/docker-compose.3-node.yml"
fi
compose_file="${compose_file:-${COMPOSE_FILE:-$default_compose}}"

if [ "$command" = "edge-cases" ]; then
  default_project="netos-edge"
else
  default_project="netos-demo"
  if [ "$topology" = "3" ]; then
    default_project="netos-demo-3node"
  fi
fi
project_name="${project_name:-${COMPOSE_PROJECT_NAME:-$default_project}}"

env_args=("COMPOSE_FILE=$compose_file" "COMPOSE_PROJECT_NAME=$project_name")
if [ -n "$timeout" ]; then
  env_args+=("TIMEOUT_SEC=$timeout")
fi
if [ -n "$expect_key" ]; then
  env_args+=("EXPECT_KEY=$expect_key")
fi
if [ -n "$final_node" ]; then
  env_args+=("FINAL_NODE=$final_node")
fi
if [ -n "$inspect_keys" ]; then
  env_args+=("INSPECT_KEYS=$inspect_keys")
fi
if [ -n "$target_service" ]; then
  env_args+=("TARGET_SERVICE=$target_service")
fi
if [ -n "$target_port" ]; then
  env_args+=("TARGET_PORT=$target_port")
fi
if [ -n "$sender_service" ]; then
  env_args+=("SENDER_SERVICE=$sender_service")
fi
if [ -n "$table_stats_format" ]; then
  env_args+=("TABLE_STATS_FORMAT=$table_stats_format")
fi

case "$command" in
  start)
    if ! command -v docker >/dev/null 2>&1; then
      echo "docker is required to run the demo." >&2
      exit 1
    fi

    if ! docker compose version >/dev/null 2>&1; then
      echo "docker compose v2 is required to run the demo." >&2
      exit 1
    fi

    echo "Starting NetOS demo via docker compose..."
    if [ "$detach" -eq 1 ]; then
      docker compose -f "$compose_file" -p "$project_name" up --build -d
      echo "Demo running in background (project=${project_name})."
      echo "Use: docker compose -f $compose_file -p $project_name logs -f"
    else
      docker compose -f "$compose_file" -p "$project_name" up --build
    fi
    ;;
  validate)
    if [ "$topology" = "3" ]; then
      env "${env_args[@]}" "$ROOT_DIR/scripts/validate_3node_demo.sh"
    else
      env "${env_args[@]}" "$ROOT_DIR/scripts/validate_demo.sh"
    fi
    ;;
  trace)
    env "${env_args[@]}" "$ROOT_DIR/scripts/trace_demo.sh"
    ;;
  table-stats)
    env "${env_args[@]}" "$ROOT_DIR/scripts/table_stats_demo.sh"
    ;;
  table-health)
    env "${env_args[@]}" "$ROOT_DIR/scripts/table_stats_health.sh"
    ;;
  pull-path)
    env "${env_args[@]}" "$ROOT_DIR/scripts/pull_path_demo.sh"
    ;;
  hop-story)
    env "${env_args[@]}" "$ROOT_DIR/scripts/hop_story_3node.sh"
    ;;
  inspect)
    env "${env_args[@]}" "$ROOT_DIR/scripts/inspect_demo.sh"
    ;;
  edge-cases)
    env "${env_args[@]}" "$ROOT_DIR/scripts/observe_edge_cases.sh"
    ;;
  check-topology)
    if [ ${#extra_topology_dirs[@]} -gt 0 ]; then
      "$ROOT_DIR/scripts/check_topology_env.sh" "${extra_topology_dirs[@]}"
    else
      "$ROOT_DIR/scripts/check_topology_env.sh"
    fi
    ;;
  *)
    echo "Unknown command: $command" >&2
    usage
    exit 1
    ;;
esac
