# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5.1)

project(KVStoreService C CXX)

include(../Cmake/common.cmake)

# Proto file
get_filename_component(rg_proto "../Protos/kvstore.proto" ABSOLUTE)
get_filename_component(rg_proto_path "${rg_proto}" PATH)

# Generated sources
set(rg_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/kvstore.pb.cc")
set(rg_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/kvstore.pb.h")
set(rg_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/kvstore.grpc.pb.cc")
set(rg_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/kvstore.grpc.pb.h")
add_custom_command(
      OUTPUT "${rg_proto_srcs}" "${rg_proto_hdrs}" "${rg_grpc_srcs}" "${rg_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${rg_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${rg_proto}"
      DEPENDS "${rg_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# rg_grpc_proto
add_library(rg_grpc_proto
  ${rg_grpc_srcs}
  ${rg_grpc_hdrs}
  ${rg_proto_srcs}
  ${rg_proto_hdrs})
target_link_libraries(rg_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# kvstore_client_helper
add_library(kvstore_client_helper
  "../KVStore/kvstore_client.h"
  "../KVStore/kvstore_client.cc")
target_link_libraries(kvstore_client_helper
  rg_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# kvstore_client_helper
add_library(kvstore_console_helper
  "../KVStore/kvstore_console.h"
  "../KVStore/kvstore_console.cc")
target_link_libraries(kvstore_console_helper
  rg_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_library(socket_reader
  "socket_reader.cpp"
  "socket_reader.hpp")

# api_handler_helper
set(NLOHMANN_DIR ../third_party)
include_directories(${NLOHMANN_DIR})

add_library(api_handler_helper
  "api_handler.cpp"
  "api_handler.hpp")
target_link_libraries(api_handler_helper
  kvstore_client_helper)

# Target server
add_executable(server server.cpp)
target_link_libraries(server 
  api_handler_helper 
  kvstore_console_helper
  socket_reader)


# Mail Service
add_executable(mail mail_service.cpp)
target_link_libraries(mail kvstore_client_helper)