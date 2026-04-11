cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(vm C)

set(CMAKE_C_STANDARD 23)

include(ExternalProject)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit")
    execute_process(
            COMMAND git clone --depth 1 https://github.com/LuaJIT/LuaJIT.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit"
            RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone LuaJIT")
    endif()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv")
    execute_process(
            COMMAND git clone --branch v1.48.0 https://github.com/libuv/libuv.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv"
            RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone libuv")
    endif()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet")
    execute_process(
            COMMAND git clone --depth 1 https://github.com/lsalzman/enet.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet"
            RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone enet")
    endif()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws")
    execute_process(
            COMMAND git clone --branch v4.3-stable https://github.com/warmcat/libwebsockets.git "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws"
            RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone libwebsockets")
    endif()
endif()

if(WIN32)
    ExternalProject_Add(luajit
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            BUILD_COMMAND cmd /c src/msvcbuild.bat static
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy src/lua51.lib "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
            COMMAND ${CMAKE_COMMAND} -E copy src/luajit.exe "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/bin/luajit.exe"
            COMMAND ${CMAKE_COMMAND} -E copy src/lua.h "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/lua.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/luajit.h "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luajit.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/luaconf.h "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luaconf.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/lualib.h "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/lualib.h"
            COMMAND ${CMAKE_COMMAND} -E copy src/lauxlib.h "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/lauxlib.h"
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
            BUILD_ALWAYS FALSE
    )
elseif(APPLE)
    ExternalProject_Add(luajit
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            BUILD_COMMAND make -j4 PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit BUILDMODE=static MACOSX_DEPLOYMENT_TARGET=11.0
            INSTALL_COMMAND make install PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit MACOSX_DEPLOYMENT_TARGET=11.0
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
            BUILD_ALWAYS FALSE
    )
else()
    ExternalProject_Add(luajit
            SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit"
            BUILD_IN_SOURCE TRUE
            CONFIGURE_COMMAND ""
            BUILD_COMMAND make -j4 PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit BUILDMODE=static
            INSTALL_COMMAND make install PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit
            BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
            BUILD_ALWAYS FALSE
    )
endif()

ExternalProject_Add(uv
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv"
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv
        -DBUILD_TESTING=OFF
        -DLIBUV_BUILD_SHARED=OFF
        BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv/lib/libuv.a"
        BUILD_ALWAYS FALSE
)

ExternalProject_Add(enet
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet"
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet
        BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet/lib/libenet.a"
        BUILD_ALWAYS FALSE
)

ExternalProject_Add(lws
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws"
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
        -DLWS_WITH_SHARED=OFF
        -DLWS_WITH_SSL=OFF
        -DLWS_WITHOUT_TESTAPPS=ON
        -DLWS_WITHOUT_TEST_SERVER=ON
        -DLWS_WITHOUT_TEST_PING=ON
        -DLWS_WITHOUT_TEST_CLIENT=ON
        BUILD_BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws/lib/libwebsockets.a"
        BUILD_ALWAYS FALSE
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luajit-2.1")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv/include")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet/include")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws/include")

add_library(liblua STATIC IMPORTED GLOBAL)
add_dependencies(liblua luajit)
set_target_properties(liblua PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/lib/libluajit-5.1.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luajit-2.1"
)

add_library(libuv STATIC IMPORTED GLOBAL)
add_dependencies(libuv uv)
set_target_properties(libuv PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv/lib/libuv.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv/include"
)

add_library(libenet STATIC IMPORTED GLOBAL)
add_dependencies(libenet enet)
set_target_properties(libenet PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet/lib/libenet.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet/include"
)

add_library(liblws STATIC IMPORTED GLOBAL)
add_dependencies(liblws lws)
set_target_properties(liblws PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws/lib/libwebsockets.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws/include"
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_library(vm STATIC ${SOURCES} ${HEADERS})
add_dependencies(vm luajit liblua uv libuv enet libenet lws liblws)

target_include_directories(vm PUBLIC
        include
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include/luajit-2.1"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/luajit/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/uv/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/enet/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lws/include"
)

target_compile_definitions(vm PRIVATE
        VM_PATH="${CMAKE_CURRENT_SOURCE_DIR}/core"
)

if(WIN32)
    target_link_libraries(vm PUBLIC liblua libuv libenet liblws ws2_32 iphlpapi userenv winmm)
else()
    target_link_libraries(vm PUBLIC liblua libuv libenet liblws m dl pthread)
endif()