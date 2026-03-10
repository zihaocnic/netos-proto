add_executable(netos-node
  main.cpp
  core/config.cpp
  core/config_validation.cpp
  core/logger.cpp
  core/message.cpp
  core/string_utils.cpp
  cache/redis_client.cpp
  network/address.cpp
  network/udp_transport.cpp
  tables/query_table.cpp
  tables/sync_table.cpp
  node/node.cpp
)

target_include_directories(netos-node PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(netos-node PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(netos-node PRIVATE Threads::Threads)
