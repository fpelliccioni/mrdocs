//
// This is a derivative work. originally part of the LLVM Project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdocs
//

#include <unordered_map>

#include "ToolArgs.hpp"
#include "lib/Lib/AbsoluteCompilationDatabase.hpp"
#include "lib/Lib/ConfigImpl.hpp"
#include "lib/Lib/CorpusImpl.hpp"
#include <mrdocs/Generators.hpp>
#include <mrdocs/Support/Error.hpp>
#include <mrdocs/Support/Path.hpp>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <cstdlib>
#include <iostream>


namespace clang {
namespace mrdocs {

std::string 
getCompilerInfo(std::string const& compiler) 
{
    std::string const command = compiler + " -v -E -x c++ - < /dev/null 2>&1";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    
    if ( ! pipe) 
    {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
    {
        result += buffer.data();
    }

    return result;
}

std::vector<std::string> 
parseIncludePaths(std::string const& compilerOutput) 
{
    std::vector<std::string> includePaths;
    std::istringstream stream(compilerOutput);
    std::string line;
    bool capture = false;

    while (std::getline(stream, line)) 
    {
        if (line.find("#include <...> search starts here:") != std::string::npos) 
        {
            capture = true;
            continue;
        }
        if (line.find("End of search list.") != std::string::npos) 
        {
            break;
        }
        if (capture) 
        {
            line.erase(0, line.find_first_not_of(" "));
            includePaths.push_back(line);
        }
    }

    return includePaths;
}

std::unordered_map<std::string, std::vector<std::string>> 
getCompilersDefaultIncludeDir(clang::tooling::CompilationDatabase const& compDb) 
{
    std::unordered_map<std::string, std::vector<std::string>> res;
    auto const allCommands = compDb.getAllCompileCommands();

    for (auto const& cmd : allCommands) {
        if ( ! cmd.CommandLine.empty()) {
            auto const& compilerPath = cmd.CommandLine[0];
            if (res.contains(compilerPath)) {
                continue;
            }

            try {
                std::string const compilerOutput = getCompilerInfo(compilerPath);
                std::vector<std::string> includePaths = parseIncludePaths(compilerOutput);
                res.emplace(compilerPath, std::move(includePaths));
            } catch (std::runtime_error const& e) {
                std::cerr << e.what() << std::endl;     //TODO(fernando): what is the proper way to handle this in MrDocs?
            }            
        }
    }

    return res;
}

Expected<void>
DoGenerateAction()
{
    // Get additional YAML settings from command line
    std::string extraYaml;
    {
        llvm::raw_string_ostream os(extraYaml);
        if (toolArgs.ignoreMappingFailures.getValue())
        {
            os << "ignore-failures: true\n";
        }
    }

    // Load YAML configuration file
    std::shared_ptr<ConfigImpl const> config;
    ThreadPool threadPool(toolArgs.concurrency);
    {
        MRDOCS_CHECK(toolArgs.configPath, "The config path argument is missing");
        MRDOCS_TRY(auto configFile, loadConfigFile(
            toolArgs.configPath,
            toolArgs.addonsDir,
            extraYaml,
            nullptr,
            threadPool));
        config = std::move(configFile);
    }

    // Get the generator
    MRDOCS_TRY(
        Generator const& generator,
        getGenerators().find(config->settings().generate),
        formatError(
            "the Generator \"{}\" was not found",
            config->settings().generate));

    // Load the compilation database
    MRDOCS_CHECK(toolArgs.inputPaths, "The compilation database path argument is missing");
    MRDOCS_CHECK(toolArgs.inputPaths.size() == 1,
        formatError(
            "got {} input paths where 1 was expected",
            toolArgs.inputPaths.size()));
    auto compilationsPath = files::normalizePath(toolArgs.inputPaths.front());
    std::cout << "*** compilationsPath: " << compilationsPath << "\n";
    std::string errorMessage;
    MRDOCS_TRY_MSG( 
        auto& jsonCompilations,
        tooling::JSONCompilationDatabase::loadFromFile(
            compilationsPath,
            errorMessage,
            tooling::JSONCommandLineSyntax::AutoDetect),
        std::move(errorMessage));

    // Calculate the working directory
    MRDOCS_TRY(auto absPath, files::makeAbsolute(compilationsPath));
    auto workingDir = files::getParentDir(absPath);
    std::cout << "*** workingDir: " << workingDir << "\n";

    // Normalize outputPath
    MRDOCS_CHECK(toolArgs.outputPath, "The output path argument is missing");
    toolArgs.outputPath = files::normalizePath(
        files::makeAbsolute(toolArgs.outputPath,
            (*config)->workingDir));

    // Get the default include paths for each compiler
    auto const defaultIncludePaths = getCompilersDefaultIncludeDir(jsonCompilations);
    // for (auto const& [compiler, includePaths] : defaultIncludePaths) {
    //     std::cout << "*** compiler: " << compiler << std::endl;
    //     for (auto const& path : includePaths) {
    //         std::cout << "*** path: " << path << std::endl;
    //     }
    // }

    // Convert relative paths to absolute
    AbsoluteCompilationDatabase compilations(
        workingDir, jsonCompilations, config, defaultIncludePaths);

    // Run the tool: this can take a while
    MRDOCS_TRY(
        auto corpus,
        CorpusImpl::build(
            report::Level::info, config, compilations));

    // Run the generator
    report::info("Generating docs\n");
    MRDOCS_TRY(generator.build(toolArgs.outputPath.getValue(), *corpus));
    return {};
}

} // mrdocs
} // clang
