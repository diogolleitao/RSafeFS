# Proto file
get_filename_component(FS_PROTO "../protos/fuse_operations.proto" ABSOLUTE)
get_filename_component(FS_PROTO_PATH "${FS_PROTO}" PATH)

# Generated sources from the proto file
set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
set(PROTO_SRCS "${PROTO_SRC_DIR}/fuse_operations.pb.cc")
set(PROTO_HDRS "${PROTO_SRC_DIR}/fuse_operations.pb.h")
set(GRPC_SRCS "${PROTO_SRC_DIR}/fuse_operations.grpc.pb.cc")
set(GRPC_HDRS "${PROTO_SRC_DIR}/fuse_operations.grpc.pb.h")

# Since FetchContent uses add_subdirectory under the hood, we can use
# the grpc targets directly from this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
add_custom_command(
        OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${PROTO_SRC_DIR}"
        --cpp_out "${PROTO_SRC_DIR}"
        -I "${FS_PROTO_PATH}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${FS_PROTO}"
        DEPENDS "${FS_PROTO}"
)

if (BUILD_SHARED_LIBS)
    message(STATUS "Compiling shared libraries ...")
    add_library(remote-safefs SHARED "")
else ()
    message(STATUS "Compiling static libraries ...")
    add_library(remote-safefs STATIC "")
endif (BUILD_SHARED_LIBS)

target_sources(
    remote-safefs 
    PRIVATE
    fuse_rpc/grpc/async_client.cpp
    fuse_rpc/grpc/server.cpp
    fuse_rpc/grpc/sync_client.cpp
    fuse_rpc/utils/dir_info.cpp
    layers/data_cache/drivers/lru.cpp
    layers/data_cache/drivers/rnd.cpp
    layers/data_cache/cache.cpp
    layers/data_cache/data_cache.cpp
    layers/local/local_operations.cpp
    layers/local/local.cpp
    layers/local/nfs_operations.cpp
    layers/metadata_cache/drivers/lru.cpp
    layers/metadata_cache/drivers/rnd.cpp
    layers/metadata_cache/cache.cpp
    layers/metadata_cache/metadata_cache.cpp
    layers/read_ahead/read_ahead_cache.cpp
    layers/read_ahead/read_ahead.cpp
    layers/rpc_client/rpc_client.cpp
    utils/utils.cpp
    utils/logging.cpp
    client.cpp
    config.cpp
    server.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_sources(
    remote-safefs
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include/rsafefs/common/cache/cache_manager.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/common/cache/lru_manager.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/common/cache/rnd_manager.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/grpc/async_client.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/grpc/channel.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/grpc/server.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/grpc/structs_fillers.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/grpc/sync_client.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/utils/dir_info.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/client.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_rpc/server.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/fuse_wrapper/fuse31.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/data_cache/drivers/lru.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/data_cache/drivers/rnd.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/data_cache/cache.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/data_cache/data_cache.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/local/local_operations.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/local/local.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/local/nfs_operations.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/metadata_cache/drivers/lru.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/metadata_cache/drivers/rnd.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/metadata_cache/cache.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/metadata_cache/metadata_cache.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/read_ahead/read_ahead_cache.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/read_ahead/read_ahead.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/layers/rpc_client/rpc_client.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/utils/utils.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/utils/logging.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/client.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/config.hpp
    ${PROJECT_SOURCE_DIR}/include/rsafefs/server.hpp
)

target_include_directories(
    remote-safefs
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROTO_SRC_DIR}
    ${FUSE_INCLUDE_DIRS}
    ${asio_SOURCE_DIR}/asio/include
)

target_link_libraries(
    remote-safefs
    PUBLIC
    yaml-cpp
    spdlog
    fmt
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    ${FUSE_LINK_LIBRARIES}
)

target_compile_options(
    remote-safefs
    PUBLIC
    ${FUSE_CFLAGS_OTHER}
)