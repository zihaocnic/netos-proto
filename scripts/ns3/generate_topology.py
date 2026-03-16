#!/usr/bin/env python3
"""Generate a simple NetOS topology JSON export for NS-3 scaffolding."""

import argparse
import json
from pathlib import Path
import sys


def _strip_quotes(value: str) -> str:
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        return value[1:-1]
    return value


def parse_env_file(path: Path) -> dict:
    env = {}
    for raw_line in path.read_text().splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        env[key.strip()] = _strip_quotes(value.strip())
    return env


def parse_neighbors(value: str):
    neighbors = []
    for item in value.split(","):
        token = item.strip()
        if not token:
            continue
        if ":" in token:
            neighbor_id, port = token.split(":", 1)
            neighbors.append((neighbor_id.strip(), port.strip()))
        else:
            neighbors.append((token, ""))
    return neighbors


def normalize_port(value: str):
    if value is None or value == "":
        return None
    try:
        return int(value)
    except ValueError:
        return value


def discover_env_files(input_path: Path):
    if input_path.is_file():
        return [input_path]
    if input_path.is_dir():
        env_files = sorted(input_path.glob("*/*.env"))
        if env_files:
            return env_files
        env_files = sorted(input_path.glob("*.env"))
        if env_files:
            return env_files
    return []


def build_topology(env_files):
    nodes_by_id = {}
    links = []

    for env_file in env_files:
        env = parse_env_file(env_file)
        node_id = env.get("NETOS_NODE_ID") or env_file.stem
        if node_id not in nodes_by_id:
            node = {"id": node_id, "config_file": str(env_file)}
            bind_ip = env.get("NETOS_BIND_IP")
            bind_port = normalize_port(env.get("NETOS_BIND_PORT"))
            if bind_ip:
                node["bind_ip"] = bind_ip
            if bind_port is not None:
                node["bind_port"] = bind_port
            nodes_by_id[node_id] = node

        for neighbor_id, port in parse_neighbors(env.get("NETOS_NEIGHBORS", "")):
            link = {"src": node_id, "dst": neighbor_id}
            normalized_port = normalize_port(port)
            if normalized_port is not None:
                link["dst_port"] = normalized_port
            links.append(link)

    nodes = sorted(nodes_by_id.values(), key=lambda item: item["id"])
    links = sorted(
        links,
        key=lambda item: (
            item["src"],
            item["dst"],
            str(item.get("dst_port", "")),
        ),
    )
    return {"nodes": nodes, "links": links}


def main():
    parser = argparse.ArgumentParser(
        description="Export NetOS topology env files to a simple JSON schema for NS-3 scaffolding."
    )
    parser.add_argument(
        "--input",
        "-i",
        default="infra/topology",
        help="Topology root directory or a specific .env file.",
    )
    parser.add_argument(
        "--output",
        "-o",
        help="Write JSON output to a file (default: stdout).",
    )
    args = parser.parse_args()

    input_path = Path(args.input)
    env_files = discover_env_files(input_path)
    if not env_files:
        sys.stderr.write(f"No .env files found under: {input_path}\n")
        return 1

    grouped = {}
    for env_file in env_files:
        topology_name = env_file.parent.name
        grouped.setdefault(topology_name, []).append(env_file)

    topologies = []
    for name in sorted(grouped.keys()):
        envs = sorted(grouped[name])
        topology = build_topology(envs)
        topology["name"] = name
        topology["env_files"] = [str(path) for path in envs]
        topologies.append(topology)

    payload = {
        "schema": "netos-ns3-topology-v1",
        "source": {"input": str(input_path)},
        "topologies": topologies,
    }

    output = json.dumps(payload, indent=2, sort_keys=True)
    if args.output:
        Path(args.output).write_text(output + "\n")
    else:
        print(output)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
