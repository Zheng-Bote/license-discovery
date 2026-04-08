/**
 * SPDX-FileComment: Unit tests for ClearlyDefined API client.
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file test_clearly_defined_client.cpp
 * @brief Unit tests using Catch2.
 * @version 0.1.0
 * @date 2026-04-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include "license_discovery/clearly_defined_client.hpp"

using namespace license_discovery;

TEST_CASE("Coordinate validation works correctly", "[validation]") {
    SECTION("Valid coordinates") {
        CHECK(ClearlyDefinedClient::is_valid_coordinate("git/github/curl/curl/7.88.1"));
        CHECK(ClearlyDefinedClient::is_valid_coordinate("npm/npmjs/-/body-parser/1.19.0"));
        CHECK(ClearlyDefinedClient::is_valid_coordinate("pypi/pypi/-/requests/2.25.1"));
    }

    SECTION("Invalid coordinates") {
        CHECK_FALSE(ClearlyDefinedClient::is_valid_coordinate(""));
        CHECK_FALSE(ClearlyDefinedClient::is_valid_coordinate("git/github/curl/7.88.1")); // missing namespace or something
        CHECK_FALSE(ClearlyDefinedClient::is_valid_coordinate("git/github/curl/curl/7.88.1/extra"));
    }
}

TEST_CASE("Client initialization", "[client]") {
    ClearlyDefinedClient client("https://api.clearlydefined.io");
    // Just ensure it doesn't crash on construction/destruction
}

// More advanced tests would require a mock server or dependency injection for the HTTP client.
