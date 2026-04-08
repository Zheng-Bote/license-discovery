/**
 * SPDX-FileComment: CLI tool for ClearlyDefined API client.
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.cpp
 * @brief Main entry point for the cdclient CLI.
 * @version 0.1.0
 * @date 2026-04-08
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 *
 * @license Apache-2.0
 */

#include "license_discovery/clearly_defined_client.hpp"
#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <print>

using namespace license_discovery;

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("cdclient", "0.1.0");

    program.add_argument("--base-url")
        .default_value(std::string("https://api.clearlydefined.io"))
        .help("Base URL of the ClearlyDefined API");

    program.add_argument("--github-token")
        .help("GitHub token for authentication");

    program.add_argument("--concurrency")
        .default_value(size_t(4))
        .scan<'u', size_t>()
        .help("Number of concurrent requests");

    program.add_argument("--timeout")
        .default_value(int(30000))
        .scan<'i', int>()
        .help("Timeout in milliseconds");

    program.add_argument("--pretty")
        .default_value(false)
        .implicit_value(true)
        .help("Pretty-print JSON output");

    program.add_argument("--spdx-fallback")
        .default_value(false)
        .implicit_value(true)
        .help("Map unknown licenses to NOASSERTION");

    argparse::ArgumentParser single_command("single");
    single_command.add_description("Query a single coordinate");
    single_command.add_argument("--coord")
        .required()
        .help("Component coordinate (type/provider/namespace/name/revision)");
    single_command.add_argument("--pretty")
        .default_value(false)
        .implicit_value(true)
        .help("Pretty-print JSON output");
    single_command.add_argument("--spdx-fallback")
        .default_value(false)
        .implicit_value(true)
        .help("Map unknown licenses to NOASSERTION");

    argparse::ArgumentParser batch_command("batch");
    batch_command.add_description("Query multiple coordinates from a file");
    batch_command.add_argument("--file")
        .required()
        .help("File containing coordinates (one per line)");
    batch_command.add_argument("--pretty")
        .default_value(false)
        .implicit_value(true)
        .help("Pretty-print JSON output");
    batch_command.add_argument("--spdx-fallback")
        .default_value(false)
        .implicit_value(true)
        .help("Map unknown licenses to NOASSERTION");

    program.add_subparser(single_command);
    program.add_subparser(batch_command);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::optional<std::string> token;
    if (program.present("--github-token")) {
        token = program.get<std::string>("--github-token");
    } else if (const char* env_token = std::getenv("GITHUB_TOKEN")) {
        token = std::string(env_token);
    }

    ClearlyDefinedClient client(
        program.get<std::string>("--base-url"),
        token,
        program.get<size_t>("--concurrency")
    );

    nlohmann::json result;
    if (program.is_subcommand_used("single")) {
        auto coord = single_command.get<std::string>("--coord");
        result = client.query_single(coord);
    } else if (program.is_subcommand_used("batch")) {
        auto file_path = batch_command.get<std::string>("--file");
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << file_path << std::endl;
            return 1;
        }
        std::vector<std::string> coords;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                coords.push_back(line);
            }
        }
        result = client.query_batch(coords);
    } else {
        std::cerr << "No command specified. Use 'single' or 'batch'." << std::endl;
        std::cerr << program;
        return 1;
    }

    bool pretty = false;
    if (program.is_subcommand_used("single")) {
        pretty = single_command.get<bool>("--pretty");
    } else if (program.is_subcommand_used("batch")) {
        pretty = batch_command.get<bool>("--pretty");
    }
    if (!pretty) pretty = program.get<bool>("--pretty");

    int indent = pretty ? 4 : -1;
    std::println("{}", result.dump(indent));

    return 0;
}
