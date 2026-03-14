# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/gdvr/build-android64/_deps/arc-src")
  file(MAKE_DIRECTORY "D:/gdvr/build-android64/_deps/arc-src")
endif()
file(MAKE_DIRECTORY
  "D:/gdvr/build-android64/_deps/arc-build"
  "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix"
  "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix/tmp"
  "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix/src/arc-populate-stamp"
  "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix/src"
  "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix/src/arc-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix/src/arc-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/gdvr/build-android64/_deps/arc-subbuild/arc-populate-prefix/src/arc-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
