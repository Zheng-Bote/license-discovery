/**
 * SPDX-FileComment: Header file for the ClearlyDefined API client.
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file clearly_defined_client.hpp
 * @brief Definition of the ClearlyDefinedClient class for querying license information.
 * @version 0.1.0
 * @date 2026-04-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#ifndef LICENSE_DISCOVERY_CLEARLY_DEFINED_CLIENT_HPP
#define LICENSE_DISCOVERY_CLEARLY_DEFINED_CLIENT_HPP

#include <chrono>
#include <expected>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace license_discovery {

/**
 * @struct RetryPolicy
 * @brief Configuration for exponential backoff retries.
 */
struct RetryPolicy {
    int max_attempts{3};
    std::chrono::milliseconds base_delay{100};
    double backoff_factor{2.0};
};

/**
 * @struct RateLimitConfig
 * @brief Configuration for client-side rate limiting.
 */
struct RateLimitConfig {
    size_t max_requests_per_second{0}; // 0 means no limit
};

/**
 * @class ClearlyDefinedClient
 * @brief Client to interact with the ClearlyDefined Definitions API.
 */
class ClearlyDefinedClient {
public:
    /**
     * @brief Construct a new ClearlyDefinedClient.
     * 
     * @param base_url The base URL of the ClearlyDefined API.
     * @param github_token Optional GitHub token for authentication.
     * @param concurrency Number of concurrent requests for batch queries.
     * @param rl Rate limit configuration.
     */
    explicit ClearlyDefinedClient(
        std::string base_url = "https://api.clearlydefined.io",
        std::optional<std::string> github_token = std::nullopt,
        size_t concurrency = 4,
        RateLimitConfig rl = {}
    );

    /**
     * @brief Destroy the ClearlyDefinedClient.
     */
    ~ClearlyDefinedClient();

    /**
     * @brief Query license information for a single coordinate synchronously.
     * 
     * @param coordinate The component coordinate (e.g., "git/github/curl/curl/7.88.1").
     * @return nlohmann::json JSON response conforming to the project schema.
     */
    [[nodiscard]] nlohmann::json query_single(const std::string& coordinate);

    /**
     * @brief Query license information for a single coordinate asynchronously with timeout.
     * 
     * @param coordinate The component coordinate.
     * @param timeout Timeout for the request.
     * @return nlohmann::json JSON response.
     */
    [[nodiscard]] nlohmann::json query_single_async(const std::string& coordinate, std::chrono::milliseconds timeout);

    /**
     * @brief Query license information for multiple coordinates in parallel.
     * 
     * @param coordinates List of component coordinates.
     * @return nlohmann::json JSON array of results.
     */
    [[nodiscard]] nlohmann::json query_batch(const std::vector<std::string>& coordinates);

    /**
     * @brief Set the retry policy for the client.
     * 
     * @param rp The new retry policy.
     */
    void set_retry_policy(RetryPolicy rp);

    /**
     * @brief Set the rate limit for the client.
     * 
     * @param rl The new rate limit configuration.
     */
    void set_rate_limit(RateLimitConfig rl);

    /**
     * @brief Validate if a coordinate follows the type/provider/namespace/name/revision pattern.
     * 
     * @param coordinate The coordinate to validate.
     * @return true If valid.
     * @return false If invalid.
     */
    [[nodiscard]] static bool is_valid_coordinate(std::string_view coordinate);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace license_discovery

#endif // LICENSE_DISCOVERY_CLEARLY_DEFINED_CLIENT_HPP
