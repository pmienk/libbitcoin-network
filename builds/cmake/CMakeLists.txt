###############################################################################
#  Copyright (c) 2014-2026 libbitcoin-network developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################
cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project( libbitcoin-network
  VERSION 4.0.0
  DESCRIPTION "Bitcoin P2P Network Library"
  LANGUAGES C CXX )


set( repository_root_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../" )

if ( MSVC )
  set( CMAKE_STATIC_LIBRARY_PREFIX "lib" )
  set( CMAKE_SHARED_LIBRARY_PREFIX "lib" )
endif()

#------------------------------------------------------------------------------
# Project options.
#------------------------------------------------------------------------------
option( with-ssl "Use embedded ssl library." ON )
option( with-test "Build test." ON )

#------------------------------------------------------------------------------
# Dependencies.
#------------------------------------------------------------------------------

if ( BUILD_SHARED_LIBS )
  set( bitcoin-system_USE_STATIC_LIBS "OFF" )
  set( Boost_USE_STATIC_LIBS "OFF" )
else()
  set( bitcoin-system_USE_STATIC_LIBS "ON" )
  set( Boost_USE_STATIC_LIBS "ON" )
endif()

find_package( bitcoin-system 4.0.0 REQUIRED )

find_package( Boost 1.86.0 REQUIRED
  COMPONENTS
    unit_test_framework
)
#------------------------------------------------------------------------------
# Compiler options.
#------------------------------------------------------------------------------

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

check_cxx_compiler_flag( "-Wall" HAS_FLAG_WALL )
if ( !HAS_FLAG_WALL )
    message( FATAL_ERROR "Compiler does not support '-Wall'." )
endif()

check_cxx_compiler_flag( "-Wextra" HAS_FLAG_WEXTRA )
if ( !HAS_FLAG_WEXTRA )
    message( FATAL_ERROR "Compiler does not support '-Wextra'." )
endif()

check_cxx_compiler_flag( "-Wno-reorder" HAS_FLAG_WNO-REORDER )
if ( !HAS_FLAG_WNO-REORDER )
    message( FATAL_ERROR "Compiler does not support '-Wno-reorder'." )
endif()

check_cxx_compiler_flag( "-Wno-missing-field-initializers" HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
if ( !HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
    message( FATAL_ERROR "Compiler does not support '-Wno-missing-field-initializers'." )
endif()

check_cxx_compiler_flag( "-Wno-missing-braces" HAS_FLAG_WNO-MISSING-BRACES )
if ( !HAS_FLAG_WNO-MISSING-BRACES )
    message( FATAL_ERROR "Compiler does not support '-Wno-missing-braces'." )
endif()

check_cxx_compiler_flag( "-Wno-comment" HAS_FLAG_WNO-COMMENT )
if ( !HAS_FLAG_WNO-COMMENT )
    message( FATAL_ERROR "Compiler does not support '-Wno-comment'." )
endif()

check_cxx_compiler_flag( "-Wno-deprecated-copy" HAS_FLAG_WNO-DEPRECATED-COPY )
if ( !HAS_FLAG_WNO-DEPRECATED-COPY )
    message( FATAL_ERROR "Compiler does not support '-Wno-deprecated-copy'." )
endif()

if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  check_cxx_compiler_flag( "-Wno-mismatched-tags" HAS_FLAG_WNO-MISMATCHED-TAGS )
  if ( !HAS_FLAG_WNO-MISMATCHED-TAGS )
      message( FATAL_ERROR "Compiler does not support '-Wno-mismatched-tags'." )
  endif()
endif()

check_cxx_compiler_flag( "-Wno-ignored-attributes" HAS_FLAG_WNO-IGNORED-ATTRIBUTES )
if ( !HAS_FLAG_WNO-IGNORED-ATTRIBUTES )
    message( FATAL_ERROR "Compiler does not support '-Wno-ignored-attributes'." )
endif()

check_cxx_compiler_flag( "-Wno-long-long" HAS_FLAG_WNO-LONG-LONG )
if ( !HAS_FLAG_WNO-LONG-LONG )
    message( FATAL_ERROR "Compiler does not support '-Wno-long-long'." )
endif()

if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
  check_cxx_compiler_flag( "-fno-var-tracking-assignments" HAS_FLAG_FNO-VAR-TRACKING-ASSIGNMENTS )
  if ( !HAS_FLAG_FNO-VAR-TRACKING-ASSIGNMENTS )
      message( FATAL_ERROR "Compiler does not support '-fno-var-tracking-assignments'." )
  endif()
endif()

#------------------------------------------------------------------------------
# bitcoin-network library
#------------------------------------------------------------------------------
add_library( bitcoin-network )
add_library( bitcoin::network ALIAS bitcoin-network )

target_compile_features( bitcoin-network
  PUBLIC
    cxx_std_20
)

target_compile_options( bitcoin-network
  PRIVATE
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-braces>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-comment>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-copy>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-mismatched-tags>
)

target_compile_definitions( bitcoin-network
  PUBLIC
    $<$<BOOL:${with-ssl}>:WITH_SSL WOLFSSL_USER_SETTINGS>
)

file( GLOB_RECURSE bitcoin_network_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.ipp"
)

file( GLOB_RECURSE bitcoin_network_HEADERS_SSL CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/network/ssl/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/network/ssl/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/network/ssl/*.ipp"
)

if ( !with-ssl )
  list( REMOVE_ITEM bitcoin_network_HEADERS ${bitcoin_network_HEADERS_SSL} )
endif()

file( GLOB_RECURSE bitcoin_network_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp"
)

file( GLOB_RECURSE bitcoin_network_SOURCES_SSL CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/ssl/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/ssl/*.cpp"
)

if ( !with-ssl )
  list( REMOVE_ITEM bitcoin_network_SOURCES ${bitcoin_network_SOURCES_SSL} )
endif()

target_sources( bitcoin-network
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    FILES
      ${bitcoin_network_HEADERS}
  PRIVATE
    ${bitcoin_network_SOURCES}
)

target_include_directories( bitcoin-network
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
    $<$<BOOL:${with-ssl}>:
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/network/ssl>
      $<INSTALL_INTERFACE:include/bitcoin/network/ssl>
    >
)

target_link_libraries( bitcoin-network
  PUBLIC
    bitcoin::system
)

set_target_properties(bitcoin-network
  PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME network
)

#------------------------------------------------------------------------------
# libbitcoin-network-tests tests
#------------------------------------------------------------------------------
if ( with-test )
  add_executable( libbitcoin-network-tests )

  enable_testing()

  add_test( NAME libbitcoin-network-tests
    COMMAND libbitcoin-network-tests
    --run_test=*
    --log_level=warning
    --show_progress=no
    --detect_memory_leak=0
    --report_level=no
    --build_info=yes
  )

  target_compile_options( libbitcoin-network-tests
    PRIVATE
      -Wall
      -Wextra
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-braces>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-comment>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-copy>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-ignored-attributes>
      $<$<CXX_COMPILER_ID:Clang>:-Wno-mismatched-tags>
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-long-long>
      $<$<CXX_COMPILER_ID:GNU>:-fno-var-tracking-assignments>
  )

  target_compile_definitions( libbitcoin-network-tests
    PRIVATE
      $<$<BOOL:${with-ssl}>:CERT_PREFIX="\"${repository_root_dir}/test/ssl/wolfssl/wolfcrypt/test/\"">
  )

  file( GLOB_RECURSE libbitcoin_network_tests_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.ipp"
  )

  file( GLOB_RECURSE libbitcoin_network_tests_HEADERS_SSL CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/ssl/wolfssl/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/ssl/wolfssl/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/ssl/wolfssl/*.ipp"
  )

  if ( !with-ssl )
    list( REMOVE_ITEM libbitcoin_network_tests_HEADERS ${libbitcoin_network_tests_HEADERS_SSL} )
  endif()

  file( GLOB_RECURSE libbitcoin_network_tests_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.cpp"
  )

  target_sources( libbitcoin-network-tests
    PRIVATE
      FILE_SET HEADERS
      BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../test"
      FILES
        ${libbitcoin_network_tests_HEADERS}
    PRIVATE
      ${libbitcoin_network_tests_SOURCES}
  )

  target_include_directories( libbitcoin-network-tests
    PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../test>
      $<$<BOOL:${with-ssl}>:
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../test/ssl/wolfssl>
      >
  )

  target_link_libraries( libbitcoin-network-tests
    PRIVATE
      Boost::unit_test_framework
      bitcoin::network
  )
endif()
#------------------------------------------------------------------------------
# Installation routine.
#------------------------------------------------------------------------------
include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

install( TARGETS bitcoin-network
  EXPORT bitcoin-network-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILE_SET HEADERS
)

install( EXPORT bitcoin-network-targets
  NAMESPACE bitcoin::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-network
  FILE bitcoin-network-targets.cmake
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bitcoin-network-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-network
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config-version.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-network
)
