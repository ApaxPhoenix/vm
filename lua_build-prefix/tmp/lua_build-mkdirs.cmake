# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/82805/CLionProjects/vm/vendor/luajit")
  file(MAKE_DIRECTORY "/Users/82805/CLionProjects/vm/vendor/luajit")
endif()
file(MAKE_DIRECTORY
  "/Users/82805/CLionProjects/vm/lua_build-prefix/src/lua_build-build"
  "/Users/82805/CLionProjects/vm/lua_build-prefix"
  "/Users/82805/CLionProjects/vm/lua_build-prefix/tmp"
  "/Users/82805/CLionProjects/vm/lua_build-prefix/src/lua_build-stamp"
  "/Users/82805/CLionProjects/vm/lua_build-prefix/src"
  "/Users/82805/CLionProjects/vm/lua_build-prefix/src/lua_build-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/82805/CLionProjects/vm/lua_build-prefix/src/lua_build-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/82805/CLionProjects/vm/lua_build-prefix/src/lua_build-stamp${cfgdir}") # cfgdir has leading slash
endif()
