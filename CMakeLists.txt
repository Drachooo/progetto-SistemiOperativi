cmake_minimum_required(VERSION 3.20)

project(SHA256 VERSION 1.0 LANGUAGES C)

set(CMAKE_MACOSX_RPATH 1)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wno-deprecated-declarations)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

add_library(sha256_utils STATIC src/sha256_utils.c)
target_link_libraries(sha256_utils ${OPENSSL_LIBRARIES})

add_executable(server src/server.c)
target_link_libraries(server sha256_utils Threads::Threads)

add_executable(client src/client.c)
target_link_libraries(client sha256_utils)