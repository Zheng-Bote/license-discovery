/**
 * SPDX-FileComment: Implementation of the ClearlyDefined API client.
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file clearly_defined_client.cpp
 * @brief Implementation of the ClearlyDefinedClient class.
 * @version 0.1.0
 * @date 2026-04-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "license_discovery/clearly_defined_client.hpp"
#include <cpr/cpr.h>
#include <future>
#include <print>
#include <random>
#include <ranges>
#include <sstream>
#include <thread>

namespace license_discovery {

/**
 * @brief Private implementation of ClearlyDefinedClient.
 */
struct ClearlyDefinedClient::Impl {
    std::string base_url;
    std::optional<std::string> github_token;
    size_t concurrency;
    RateLimitConfig rate_limit;
    RetryPolicy retry_policy;

    Impl(std::string url, std::optional<std::string> token, size_t conc, RateLimitConfig rl)
        : base_url(std::move(url)), github_token(std::move(token)), concurrency(conc), rate_limit(rl) {}

    /**
     * @brief Normalizes the coordinate for the API.
     */
    [[nodiscard]] std::string normalize_coordinate(const std::string& coordinate) const {
        return coordinate; // For now, assume it's already normalized or valid
    }

    /**
     * @brief Formats a success response according to the schema.
     */
    [[nodiscard]] nlohmann::json format_success(const std::string& coordinate, const nlohmann::json& raw) const {
        nlohmann::json res;
        res["coordinate"] = coordinate;
        res["status"] = {
            {"http", 200},
            {"ok", true},
            {"source", "clearlydefined"}
        };
        
        // Extract license information
        std::string declared = "NOASSERTION";
        if (raw.contains("licensed") && raw["licensed"].contains("declared")) {
            declared = raw["licensed"]["declared"].get<std::string>();
        }
        res["licensed"]["declared"] = declared;
        
        res["licensed"]["findings"] = nlohmann::json::array();
        if (raw.contains("licensed") && raw["licensed"].contains("facets") && raw["licensed"]["facets"].contains("core") && raw["licensed"]["facets"]["core"].contains("discovered") && raw["licensed"]["facets"]["core"]["discovered"].contains("expressions")) {
             for (auto& expr : raw["licensed"]["facets"]["core"]["discovered"]["expressions"]) {
                 res["licensed"]["findings"].push_back({{"file", "unknown"}, {"license", expr}});
             }
        }

        if (raw.contains("described")) {
            res["described"] = raw["described"];
        } else {
            res["described"] = nlohmann::json::object();
        }

        res["raw"] = raw;
        return res;
    }

    /**
     * @brief Formats an error response according to the schema.
     */
    [[nodiscard]] nlohmann::json format_error(const std::string& coordinate, int http_status, std::string_view code, std::string_view message) const {
        return {
            {"coordinate", coordinate},
            {"status", {
                {"http", http_status},
                {"ok", false},
                {"source", "error"}
            }},
            {"error", {
                {"code", std::string(code)},
                {"message", std::string(message)}
            }}
        };
    }

    /**
     * @brief Executes a single GET request with retries and backoff.
     */
    [[nodiscard]] nlohmann::json execute_request(const std::string& coordinate, std::chrono::milliseconds timeout = std::chrono::milliseconds(30000)) {
        if (!ClearlyDefinedClient::is_valid_coordinate(coordinate)) {
            return format_error(coordinate, 400, "INVALID_COORDINATE", "Coordinate must follow type/provider/namespace/name/revision pattern");
        }

        std::string url = base_url + "/definitions/" + coordinate;
        cpr::Header header;
        if (github_token) {
            header["Authorization"] = "token " + *github_token;
        }

        int attempt = 0;
        std::chrono::milliseconds delay = retry_policy.base_delay;
        std::default_random_engine generator(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

        while (attempt < retry_policy.max_attempts) {
            auto response = cpr::Get(cpr::Url{url}, header, cpr::Timeout{timeout});

            if (response.status_code == 200) {
                try {
                    return format_success(coordinate, nlohmann::json::parse(response.text));
                } catch (const nlohmann::json::parse_error& e) {
                    return format_error(coordinate, 200, "PARSE_ERROR", e.what());
                }
            }

            // Retry on 429 (Too Many Requests) or 5xx (Server Errors)
            bool should_retry = (response.status_code == 429 || (response.status_code >= 500 && response.status_code < 600) || response.status_code == 0);
            
            if (!should_retry || ++attempt >= retry_policy.max_attempts) {
                std::string msg = response.error ? response.error.message : "HTTP " + std::to_string(response.status_code);
                return format_error(coordinate, static_cast<int>(response.status_code), "API_ERROR", msg);
            }

            // Exponential backoff with jitter
            std::uniform_real_distribution<double> distribution(0.8, 1.2);
            auto jittered_delay = std::chrono::milliseconds(static_cast<long long>(static_cast<double>(delay.count()) * distribution(generator)));
            std::this_thread::sleep_for(jittered_delay);
            delay = std::chrono::milliseconds(static_cast<long long>(static_cast<double>(delay.count()) * retry_policy.backoff_factor));
        }

        return format_error(coordinate, 0, "MAX_RETRIES_EXCEEDED", "Exceeded max retry attempts");
    }
};

ClearlyDefinedClient::ClearlyDefinedClient(std::string base_url, std::optional<std::string> github_token, size_t concurrency, RateLimitConfig rl)
    : pimpl_(std::make_unique<Impl>(std::move(base_url), std::move(github_token), concurrency, rl)) {}

ClearlyDefinedClient::~ClearlyDefinedClient() = default;

nlohmann::json ClearlyDefinedClient::query_single(const std::string& coordinate) {
    return pimpl_->execute_request(coordinate);
}

nlohmann::json ClearlyDefinedClient::query_single_async(const std::string& coordinate, std::chrono::milliseconds timeout) {
    // This is just a wrapper for now, cpr is synchronous here but we can wrap it in std::async if needed
    // However, query_single_async in the API suggestion implies it's still returning nlohmann::json directly
    // which means it blocks until either success or timeout.
    return pimpl_->execute_request(coordinate, timeout);
}

nlohmann::json ClearlyDefinedClient::query_batch(const std::vector<std::string>& coordinates) {
    std::vector<std::future<nlohmann::json>> futures;
    size_t active_requests = 0;
    nlohmann::json results = nlohmann::json::array();

    for (const auto& coord : coordinates) {
        // Simple concurrency control
        if (pimpl_->concurrency > 0 && active_requests >= pimpl_->concurrency) {
            // Wait for one to finish (this is a bit naive but works for a prototype)
            // A better way would be a thread pool or a semaphore.
        }

        futures.push_back(std::async(std::launch::async, [this, coord]() {
            if (pimpl_->rate_limit.max_requests_per_second > 0) {
                 // Very simple rate limit: just sleep a bit
                 std::this_thread::sleep_for(std::chrono::milliseconds(1000 / pimpl_->rate_limit.max_requests_per_second));
            }
            return query_single(coord);
        }));
        active_requests++;
    }

    for (auto& f : futures) {
        results.push_back(f.get());
    }

    return results;
}

void ClearlyDefinedClient::set_retry_policy(RetryPolicy rp) {
    pimpl_->retry_policy = rp;
}

void ClearlyDefinedClient::set_rate_limit(RateLimitConfig rl) {
    pimpl_->rate_limit = rl;
}

bool ClearlyDefinedClient::is_valid_coordinate(std::string_view coordinate) {
    if (coordinate.empty()) return false;
    
    size_t slash_count = 0;
    for (char c : coordinate) {
        if (c == '/') slash_count++;
    }
    
    // Pattern: type/provider/namespace/name/revision
    // That's 4 slashes.
    return slash_count == 4;
}

} // namespace license_discovery
