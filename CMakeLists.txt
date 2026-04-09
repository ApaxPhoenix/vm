cmake_minimum_required(VERSION 3.20)
project(vm C)

set(CMAKE_C_STANDARD 23)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/vendor/luajit/src/lua.h")
    message(STATUS "LuaJIT not found, cloning...")
    execute_process(
            COMMAND git clone https://github.com/LuaJIT/LuaJIT.git
            ${CMAKE_SOURCE_DIR}/vendor/luajit
            RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone LuaJIT")
    endif()
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/luajit_install/include/luajit-2.1")

include(ExternalProject)
ExternalProject_Add(luajit
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/vendor/luajit"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make -j4 PREFIX=${CMAKE_BINARY_DIR}/luajit_install BUILDMODE=static MACOSX_DEPLOYMENT_TARGET=11.0
        INSTALL_COMMAND make install PREFIX=${CMAKE_BINARY_DIR}/luajit_install MACOSX_DEPLOYMENT_TARGET=11.0
        BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/luajit_install/lib/libluajit-5.1.a"
        BUILD_ALWAYS FALSE
)

add_library(lua_static STATIC IMPORTED GLOBAL)
add_dependencies(lua_static luajit)
set_target_properties(lua_static PROPERTIES
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/luajit_install/lib/libluajit-5.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/luajit_install/include/luajit-2.1"
)

file(GLOB_RECURSE VM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE VM_INCLUDES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/include/*.h"
)

add_library(vm STATIC ${VM_SOURCES})

add_dependencies(vm lua_static)

target_include_directories(vm PUBLIC
        include
        "${CMAKE_BINARY_DIR}/luajit_install/include/luajit-2.1"
)

file(COPY "${CMAKE_SOURCE_DIR}/server.lua"
        DESTINATION "${CMAKE_BINARY_DIR}/vendor/vm")

file(COPY "${CMAKE_SOURCE_DIR}/client.lua"
        DESTINATION "${CMAKE_BINARY_DIR}/vendor/vm")

file(COPY "${CMAKE_SOURCE_DIR}/signal.lua"
        DESTINATION "${CMAKE_BINARY_DIR}/vendor/vm")

target_link_libraries(vm PUBLIC lua_static m dl pthread)