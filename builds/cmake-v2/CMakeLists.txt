#cmake_minimum_required(VERSION 3.22...3.30)
cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(libbitcoin-network
  VERSION 4.0.0
  DESCRIPTION "Bitcoin Cross-Platform C++ Development Toolkit - base library"
  LANGUAGES C CXX
)

#set(Boost_VERBOSE true)

if (MSVC)
  set( CMAKE_STATIC_LIBRARY_PREFIX "lib" )
  set( CMAKE_SHARED_LIBRARY_PREFIX "lib" )
endif ()

add_library( bitcoin-network )
add_library( bitcoin::network ALIAS bitcoin-network )

# ──────────────────────────────────────────────────────────────
#   Options
# ─────────────────────────────────────────────────────────────

#option(BUILD_SHARED_LIBS "Build shared library instead of static" OFF)
option(with-tests "Build unit tests" ON)
option(with-ssl "Build with internal ssl." ON)
set(LIBBITCOIN_CXX_STANDARD 20 CACHE STRING "Require C++ 20 standard.")

# ──────────────────────────────────────────────────────────────
#   Dependencies
# ──────────────────────────────────────────────────────────────

find_package(Boost 1.86.0 REQUIRED
  COMPONENTS
    unit_test_framework
)

find_package( bitcoin-system 4.0.0 REQUIRED )

# ──────────────────────────────────────────────────────────────
#   Library definition
# ──────────────────────────────────────────────────────────────

file(GLOB_RECURSE bitcoin_network_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/*.ipp"
)

file( GLOB_RECURSE bitcoin_network_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
)

target_sources( bitcoin-network PUBLIC
  FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  FILES ${bitcoin_network_HEADERS}
)

target_sources( bitcoin-network PRIVATE
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

target_compile_features(bitcoin-network PUBLIC cxx_std_${LIBBITCOIN_CXX_STANDARD})

target_compile_definitions(bitcoin-network
  PRIVATE
    $<$<BOOL:${BUILD_SHARED_LIBS}>:BC_DLL>
    # Add other internal defines if needed, e.g. BC_USE_ICU
)

target_link_libraries(bitcoin-network
  PUBLIC
    bitcoin::system
)

# Very common flags for libbitcoin projects
target_compile_options(bitcoin-network
  PUBLIC
    $<$<BOOL:${with-ssl}>:-DWITH_SSL -DWOLFSSL_USER_SETTINGS>
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive- /W3>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

set_target_properties(bitcoin-network PROPERTIES
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME system
)

# ──────────────────────────────────────────────────────────────
#   Tests
# ──────────────────────────────────────────────────────────────

if(with-tests)
  enable_testing()

  file(GLOB_RECURSE bitcoin_network_test_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.cpp"
  )

  add_executable( bitcoin-network-test )

  target_sources( bitcoin-network-test PRIVATE
    ${bitcoin_network_test_SOURCES} )

target_compile_options(bitcoin-network-test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive- /W3>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
    $<BOOL:${with-ssl}>:-DCERT_PREFIX="\\\"${CMAKE_CURRENT_SOURCE_DIR}/../../test/wolfssl/wolfcrypt/test/\\\""
)

  target_link_libraries( bitcoin-network-test PRIVATE
    bitcoin::network
    Boost::unit_test_framework
  )

  target_include_directories( bitcoin-network-test PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<$<BOOL:${with-ssl}>:$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/network/ssl>>
  )

  add_test( NAME bitcoin-network-test COMMAND bitcoin-network-test
    --run_test=*
    --log_level=warning
    --show_progress=no
    --detect_memory_leak=0
    --report_level=no
    --build_info=yes
  )

endif()

# ──────────────────────────────────────────────────────────────
#   Installation & CMake package config
# ──────────────────────────────────────────────────────────────

include(GNUInstallDirs)

install(TARGETS bitcoin-network
  EXPORT bitcoin-network-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILE_SET HEADERS
)

install(EXPORT bitcoin-network-targets
  NAMESPACE bitcoin::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-network
  FILE bitcoin-network-targets.cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bitcoin-network-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-network
)

write_basic_package_version_file(
  bitcoin-network-config-version.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-network-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-network
)
