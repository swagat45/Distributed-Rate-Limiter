cmake_minimum_required(VERSION 3.16)
project(des_rate_limiter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(prometheus-cpp REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GRPC REQUIRED IMPORTED_TARGET grpc++)
pkg_check_modules(HIREDIS REQUIRED IMPORTED_TARGET hiredis)

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/ratelimit.proto)
set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
file(MAKE_DIRECTORY ${GENERATED_PROTO_DIR})
set(PROTO_SRC ${GENERATED_PROTO_DIR}/ratelimit.pb.cc)
set(PROTO_HDR ${GENERATED_PROTO_DIR}/ratelimit.pb.h)
set(GRPC_SRC ${GENERATED_PROTO_DIR}/ratelimit.grpc.pb.cc)
set(GRPC_HDR ${GENERATED_PROTO_DIR}/ratelimit.grpc.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRC} ${PROTO_HDR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         -I ${CMAKE_CURRENT_SOURCE_DIR}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    VERBATIM
)

add_custom_command(
    OUTPUT ${GRPC_SRC} ${GRPC_HDR}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         -I ${CMAKE_CURRENT_SOURCE_DIR}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    VERBATIM
)

add_library(proto_lib ${PROTO_SRC} ${GRPC_SRC})
target_link_libraries(proto_lib
    PUBLIC
        protobuf::libprotobuf
        PkgConfig::GRPC
)
target_include_directories(proto_lib
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(ratelimiter_lib
    src/metrics.cpp
    src/ratelimiter_service_impl.cpp
    src/redis_store.cpp
)
target_link_libraries(ratelimiter_lib
    PUBLIC
        proto_lib
        PkgConfig::HIREDIS
        prometheus-cpp::core
        prometheus-cpp::pull
        Threads::Threads
)
target_include_directories(ratelimiter_lib
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(server src/server.cpp)
target_link_libraries(server PRIVATE ratelimiter_lib)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE proto_lib)

option(WITH_RAFT "Enable Raft backend" OFF)

if(WITH_RAFT)
    add_definitions(-DWITH_RAFT)
    add_library(raft_store src/raft_store.cpp)
    target_link_libraries(raft_store PRIVATE proto_lib)
endif()

enable_testing()
file(GLOB TEST_SRC tests/*.cpp)
add_executable(unit_tests ${TEST_SRC})
target_link_libraries(unit_tests PRIVATE ratelimiter_lib GTest::gtest_main)
add_test(NAME unit COMMAND unit_tests)
