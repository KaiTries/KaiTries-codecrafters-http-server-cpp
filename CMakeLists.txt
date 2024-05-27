cmake_minimum_required(VERSION 3.13)
project(http-server-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

set(SOURCE_FILES src/server.cpp)

add_executable(server ${SOURCE_FILES})

# adds support for pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(server PRIVATE ZLIB::ZLIB)