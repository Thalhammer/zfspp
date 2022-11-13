cmake_minimum_required(VERSION 3.15)

project(Zfspp)
find_package(Threads REQUIRED)

option(ZFSPP_BUILD_TEST "Enable test builds" OFF)
option(ZFSPP_WITH_ASAN "Enable asan for test builds" ON)

find_package(PkgConfig REQUIRED)
pkg_search_module(libzfs IMPORTED_TARGET libzfs REQUIRED)
set_target_properties(PkgConfig::libzfs PROPERTIES IMPORTED_GLOBAL TRUE)

add_library(zfspp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dataset.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/event_watcher.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nvlist.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pool.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/zfs.cpp
)
target_link_libraries(zfspp PUBLIC PkgConfig::libzfs Threads::Threads)
target_include_directories(zfspp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(zfspp PUBLIC cxx_std_11)

if(ZFSPP_BUILD_TEST)
  enable_testing()
  include(GoogleTest)
  if(HUNTER_ENABLED)
    hunter_add_package(GTest)
    find_package(GTest CONFIG REQUIRED)
  else()
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(
    zfspp-test
    ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)
  target_link_libraries(zfspp-test PRIVATE zfspp GTest::gtest
                                             GTest::gtest_main Threads::Threads)

  if(ZFSPP_WITH_ASAN)
    message(STATUS "Building with asan enabled")
    target_compile_options(zfspp-test PRIVATE -fsanitize=address)
    target_link_libraries(zfspp-test PRIVATE -fsanitize=address)
  endif()

  gtest_discover_tests(zfspp-test)
endif()