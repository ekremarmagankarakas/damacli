cmake_minimum_required(VERSION 3.20)
project(damacli CXX)

# doctest's CMake declares an older minimum; allow it.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Engine sources: all src/*.cc except main.cc and the public facade.
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cc")
list(REMOVE_ITEM ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/public/dama.cc"
)

# Internal static library used by the CLI executable and tests.
add_library(damacli_engine STATIC ${ENGINE_SOURCES})
target_include_directories(damacli_engine PUBLIC src)

# Public-facing static library: same engine sources plus the pimpl facade.
# Consumers only see src/public (which contains dama.h); all engine headers
# stay private.
add_library(damacli_lib STATIC
    ${ENGINE_SOURCES}
    src/public/dama.cc
)
target_include_directories(damacli_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/src/public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(damacli_lib PUBLIC cxx_std_20)
add_library(dama::dama ALIAS damacli_lib)

# Main executable.
add_executable(damacli src/main.cc)
target_link_libraries(damacli PRIVATE damacli_engine)

# Tests (opt-in). doctest fetched via FetchContent.
option(DAMACLI_BUILD_TESTS "Build unit tests" ON)
if(DAMACLI_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  include(FetchContent)
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
  )
  FetchContent_MakeAvailable(doctest)

  file(GLOB TEST_SOURCES "tests/*.cc")
  if(TEST_SOURCES)
    add_executable(damacli_tests ${TEST_SOURCES})
    target_link_libraries(damacli_tests PRIVATE damacli_engine doctest::doctest)

    enable_testing()
    add_test(NAME unit COMMAND damacli_tests)
  endif()
endif()
