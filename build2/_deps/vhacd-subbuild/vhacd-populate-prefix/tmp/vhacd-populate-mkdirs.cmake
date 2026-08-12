# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-src")
  file(MAKE_DIRECTORY "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-src")
endif()
file(MAKE_DIRECTORY
  "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-build"
  "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix"
  "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix/tmp"
  "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix/src/vhacd-populate-stamp"
  "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix/src"
  "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix/src/vhacd-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix/src/vhacd-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/zacharyzhang/Documents/GitHub/Earthcall/build2/_deps/vhacd-subbuild/vhacd-populate-prefix/src/vhacd-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
