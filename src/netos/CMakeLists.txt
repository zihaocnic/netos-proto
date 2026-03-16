add_executable(netos-node
  main.cpp
  core/config.cpp
  core/config_validation.cpp
  core/logger.cpp
  core/message.cpp
  core/string_utils.cpp
  cache/redis_client.cpp
  network/address.cpp
  network/topology.cpp
  network/udp_transport.cpp
  network/udp_network.cpp
  tables/bloom_filter.cpp
  tables/content_bf_table.cpp
  tables/content_bf_fallback.cpp
  tables/query_bf_aggregator.cpp
  tables/query_bf_limiter.cpp
  tables/query_table.cpp
  tables/sync_table.cpp
  node/data_pipeline.cpp
  node/forward_queue.cpp
  node/node.cpp
  node/request_pipeline.cpp
)

target_include_directories(netos-node PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(netos-node PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(netos-node PRIVATE Threads::Threads)

if (BUILD_TESTING)
  add_executable(netos-core-tests
    tests/core_smoke.cpp
    core/config_validation.cpp
    core/message.cpp
    core/string_utils.cpp
  )

  target_include_directories(netos-core-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(netos-core-tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME netos-core-tests COMMAND netos-core-tests)
endif()
