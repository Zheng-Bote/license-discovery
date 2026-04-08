cmake_minimum_required(VERSION 3.28)
project(license-discovery
VERSION 0.1.0 
DESCRIPTION "A C++23 library and CLI tool to query the ClearlyDefined Definitions API for component license information."
HOMEPAGE_URL "https://github.com/Zheng-Bote/license-discovery"
LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /std:c++latest)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

# Options
if(PROJECT_IS_TOP_LEVEL)
    option(BUILD_TESTING "Build unit tests" ON)
    option(ENABLE_CLI "Build CLI" ON)
else()
    option(BUILD_TESTING "Build unit tests" OFF)
    option(ENABLE_CLI "Build CLI" OFF)
endif()

# Find dependencies
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)

# Include directory
include_directories(include)

# Library
add_library(license_discovery
    src/clearly_defined_client.cpp
)

add_library(license_discovery::license_discovery ALIAS license_discovery)

target_link_libraries(license_discovery
    PUBLIC
    cpr::cpr
    nlohmann_json::nlohmann_json
)

target_include_directories(license_discovery
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# CLI
if(ENABLE_CLI)
    add_executable(cdclient src/main.cpp)
    target_link_libraries(cdclient
        PRIVATE
        license_discovery
        argparse::argparse
    )
endif()

# Tests
if(BUILD_TESTING)
    enable_testing()
    find_package(Catch2 CONFIG REQUIRED)
    
    add_executable(tests_license_discovery 
        tests/test_clearly_defined_client.cpp
    )
    target_link_libraries(tests_license_discovery
        PRIVATE
        license_discovery
        Catch2::Catch2WithMain
    )
    include(CTest)
    add_test(NAME tests_license_discovery COMMAND tests_license_discovery)
endif()

# Install
install(TARGETS license_discovery
    EXPORT license_discoveryTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

if(ENABLE_CLI)
    install(TARGETS cdclient RUNTIME DESTINATION bin)
endif()
