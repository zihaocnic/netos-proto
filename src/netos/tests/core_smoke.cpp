#include "core/config_validation.h"
#include "core/message.h"
#include "core/string_utils.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const std::string& label) {
  if (condition) {
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << label << "\n";
}

void expect_eq(const std::string& actual, const std::string& expected, const std::string& label) {
  if (actual == expected) {
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << label << " (expected '" << expected << "', got '" << actual << "')\n";
}
}

int main() {
  using netos::Config;
  using netos::Message;
  using netos::MessageType;
  using netos::NeighborConfig;
  using netos::split;
  using netos::trim;
  using netos::validate_config;

  expect_eq(trim("  alpha  "), "alpha", "trim strips whitespace");
  auto parts_keep = split("a,,b", ',', false);
  expect(parts_keep.size() == 3, "split keeps empty tokens");
  expect_eq(parts_keep[0], "a", "split keeps token a");
  expect_eq(parts_keep[1], "", "split keeps empty token");
  expect_eq(parts_keep[2], "b", "split keeps token b");

  auto parts_skip = split("a,,b", ',', true);
  expect(parts_skip.size() == 2, "split skips empty tokens");
  expect_eq(parts_skip[0], "a", "split skips token a");
  expect_eq(parts_skip[1], "b", "split skips token b");

  Message req;
  req.type = MessageType::Request;
  req.request_id = "rid";
  req.origin = "node1";
  req.ttl = 3;
  req.key = "alpha";
  auto req_wire = req.to_wire();
  auto req_parsed = Message::from_wire(req_wire);
  expect(req_parsed.has_value(), "request roundtrip parses");
  if (req_parsed.has_value()) {
    expect(req_parsed->type == MessageType::Request, "request roundtrip type");
    expect_eq(req_parsed->request_id, "rid", "request roundtrip id");
    expect_eq(req_parsed->origin, "node1", "request roundtrip origin");
    expect(req_parsed->ttl == 3, "request roundtrip ttl");
    expect_eq(req_parsed->key, "alpha", "request roundtrip key");
    expect_eq(req_parsed->value, "", "request roundtrip empty value");
  }

  auto req_min = Message::from_wire("REQ|rid2|node2|2|beta");
  expect(req_min.has_value(), "request minimal wire parses");
  if (req_min.has_value()) {
    expect(req_min->value.empty(), "request minimal value empty");
  }

  auto data = Message::from_wire("DATA|rid3|node3|2|gamma|value");
  expect(data.has_value(), "data wire parses");
  if (data.has_value()) {
    expect(data->type == MessageType::Data, "data wire type");
    expect_eq(data->value, "value", "data wire value");
  }

  auto unknown = Message::from_wire("BOGUS|rid4|node4|1|k|v");
  expect(unknown.has_value(), "unknown wire parses");
  if (unknown.has_value()) {
    expect(unknown->type == MessageType::Unknown, "unknown wire type");
  }

  Config cfg;
  cfg.node_id = "node1";
  cfg.bind_port = 9000;
  cfg.request_ttl = 3;
  cfg.request_delay_ms = 0;
  cfg.query_ttl_ms = 1500;
  cfg.sync_table_capacity = 8;
  NeighborConfig neighbor;
  neighbor.host = "127.0.0.1";
  neighbor.port = 9001;
  cfg.neighbors.push_back(neighbor);

  std::string error;
  expect(validate_config(cfg, &error), "valid config passes");

  Config bad_ttl = cfg;
  bad_ttl.request_ttl = 0;
  error.clear();
  expect(!validate_config(bad_ttl, &error), "invalid ttl rejected");
  expect_eq(error, "request_ttl must be positive", "invalid ttl error message");

  Config bad_neighbor = cfg;
  bad_neighbor.neighbors[0].port = 0;
  error.clear();
  expect(!validate_config(bad_neighbor, &error), "invalid neighbor rejected");
  expect_eq(error, "neighbor port must be between 1 and 65535", "invalid neighbor error message");

  if (failures == 0) {
    std::cout << "netos core tests: ok\n";
    return 0;
  }
  std::cout << "netos core tests: " << failures << " failures\n";
  return 1;
}
