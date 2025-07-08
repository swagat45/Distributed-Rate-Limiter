
cmake_minimum_required(VERSION 3.16)
project(des_rate_limiter)

set(CMAKE_CXX_STANDARD 17)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(hiredis REQUIRED)
find_package(prometheus-cpp REQUIRED)
find_package(Threads REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})

file(GLOB PROTO_FILES proto/*.proto)
protobuf_generate_cpp(PROTO_SRC PROTO_HDR ${PROTO_FILES})
grpc_generate_cpp(GRPC_SRC GRPC_HDR ${PROTO_FILES})

add_library(proto_lib ${PROTO_SRC} ${GRPC_SRC})
target_link_libraries(proto_lib PUBLIC protobuf::libprotobuf gRPC::grpc++)

file(GLOB SRC src/*.cpp)
add_executable(server ${SRC})
target_link_libraries(server PRIVATE proto_lib hiredis prometheus-cpp::core prometheus-cpp::pull Threads::Threads)

add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE proto_lib)

option(WITH_RAFT "Enable Raft backend" OFF)

find_package(GTest REQUIRED)

if(WITH_RAFT)
    add_definitions(-DWITH_RAFT)
    add_library(raft_store src/raft_store.cpp)
    target_link_libraries(raft_store PRIVATE proto_lib)
endif()

enable_testing()
file(GLOB TEST_SRC tests/*.cpp)
add_executable(unit_tests ${TEST_SRC} src/token_bucket.cpp)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main)
add_test(NAME unit COMMAND unit_tests)
