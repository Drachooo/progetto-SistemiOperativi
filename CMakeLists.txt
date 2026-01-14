cmake_minimum_required(VERSION 3.20)

project(SHA256 VERSION 1.0)

set(CMAKE_MACOSX_RPATH 1)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wno-deprecated-declarations)

find_package(OpenSSL REQUIRED)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/include ${OPENSSL_INCLUDE_DIR})

add_executable(sha256_string src/sha256_string.c)
target_link_libraries(sha256_string ${OPENSSL_LIBRARIES})

add_executable(sha256_file src/sha256_file.c)
target_link_libraries(sha256_file ${OPENSSL_LIBRARIES})