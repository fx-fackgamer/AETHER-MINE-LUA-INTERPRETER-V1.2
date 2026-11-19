
cmake_minimum_required(VERSION 3.22.1)

project(aetherlua)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lua")

file(GLOB LUA_SOURCES "${LUA_DIR}/*.c")

if(NOT LUA_SOURCES)
    message(FATAL_ERROR
        "Lua 5.4 source files not found!")
endif()

list(FILTER LUA_SOURCES
    EXCLUDE REGEX ".*/(lua|luac)\\.c$"
)

add_library(lua54 STATIC ${LUA_SOURCES})

target_include_directories(
    lua54 PUBLIC "${LUA_DIR}"
)

add_library(aetherlua SHARED native-lib.cpp)

target_include_directories(
    aetherlua PRIVATE "${LUA_DIR}"
)

find_library(log-lib log)

target_link_libraries(
    aetherlua
    lua54
    ${log-lib}
)
