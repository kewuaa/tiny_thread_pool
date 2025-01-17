CMAKE_MINIMUM_REQUIRED(VERSION 3.15)

PROJECT(
    tiny_thread_pool
    LANGUAGES CXX
)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_CXX_STANDARD 17)

ADD_LIBRARY(tiny_thread_pool ${CMAKE_BUILD_TYPE})
TARGET_SOURCES(
    tiny_thread_pool
    PRIVATE "src/tiny_thread_pool.cpp"
)
TARGET_INCLUDE_DIRECTORIES(
    tiny_thread_pool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

INSTALL(
    TARGETS tiny_thread_pool
    EXPORT TinyThreadPool
    LIBRARY DESTINATION lib
)
INSTALL(
    FILES src/tiny_thread_pool.hpp
    DESTINATION include
)
INSTALL(
    EXPORT TinyThreadPool
    FILE tiny_thread_pool-config.cmake
    NAMESPACE TinyThreadPool::
    DESTINATION lib/cmake/tiny_thread_pool
)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    ADD_EXECUTABLE(test_tiny_thread_pool "tests/test_tiny_thread_pool.cpp")
    TARGET_LINK_LIBRARIES(test_tiny_thread_pool PRIVATE tiny_thread_pool)

    ENABLE_TESTING()
    ADD_TEST(
        NAME test_tiny_thread_pool
        COMMAND $<TARGET_FILE:test_tiny_thread_pool>
    )
endif()
