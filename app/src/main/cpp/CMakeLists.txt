
cmake_minimum_required(VERSION 3.22.1)

project(aetherlua LANGUAGES C CXX)

# Language standards
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Lua 5.4.9 source directory
set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lua")

# Collect Lua source files
file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
    "${LUA_DIR}/*.c"
)

if(NOT LUA_SOURCES)
    message(FATAL_ERROR
        "Lua 5.4.9 source files not found in ${LUA_DIR}"
    )
endif()

# Exclude standalone executables and optional test sources
list(FILTER LUA_SOURCES EXCLUDE REGEX
    ".*/(lua|luac|onelua|ltests)\\.c$"
)

if(NOT LUA_SOURCES)
    message(FATAL_ERROR
        "No usable Lua source files found!"
    )
endif()

# Lua 5.4 Static Library
add_library(lua54 STATIC
    ${LUA_SOURCES}
)

target_include_directories(lua54 PUBLIC
    "${LUA_DIR}"
)

set_target_properties(lua54 PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Android Native Library
add_library(aetherlua SHARED
    native-lib.cpp
)

target_include_directories(aetherlua PRIVATE
    "${LUA_DIR}"
)

# Android system libraries
find_library(log-lib log)
find_library(dl-lib dl)
find_library(m-lib m)

# Link Lua Runtime with Android Native Library
target_link_libraries(aetherlua
    PRIVATE
    lua54
    ${log-lib}
    ${dl-lib}
    ${m-lib}
)
