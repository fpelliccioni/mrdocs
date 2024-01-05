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

#include "CompilerInfo.hpp"
#include "ToolArgs.hpp"
#include "lib/Lib/ConfigImpl.hpp"
#include "lib/Lib/CorpusImpl.hpp"
#include "lib/Lib/MrDocsCompilationDatabase.hpp"
#include "llvm/Support/Program.h"
#include <mrdocs/Generators.hpp>
#include <mrdocs/Support/Error.hpp>
#include <mrdocs/Support/Path.hpp>
#include <clang/Tooling/JSONCompilationDatabase.h>

#include <cstdlib>

namespace clang {
namespace mrdocs {

std::string getCurrentWorkingDirectory()
{
    namespace fs = llvm::sys::fs;
    auto result = llvm::SmallVector<char, 256>();
    if (fs::current_path(result))
    {
        return std::string(result.begin(), result.end());
    }
    return {};
}

Expected<std::string>
generateCompilationDatabaseIfNeeded(llvm::StringRef path)
{
    namespace fs = llvm::sys::fs;
    namespace path = llvm::sys::path;

    fs::file_status fileStatus;
    if (auto ec = fs::status(path, fileStatus))
    {
        return Unexpected(Error(ec));
    }

    if (fs::is_directory(fileStatus))
    {
        return executeCmakeExportCompileCommands(path);
    }
    else if (fs::is_regular_file(fileStatus))
    {
        auto const filePath = path::filename(path)
        if (filePath == "compile_commands.json")
        {
            return path;
        }
        else if (filePath == "CMakeLists.txt")
        {
            return executeCmakeExportCompileCommands(filePath.parent_path().string());
        }
    }
    return path;
}

Expected<void>
DoGenerateAction()
{
    // --------------------------------------------------------------
    //
    // Load configuration
    //
    // --------------------------------------------------------------
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

    // --------------------------------------------------------------
    //
    // Load generator
    //
    // --------------------------------------------------------------
    MRDOCS_TRY(
        Generator const& generator,
        getGenerators().find(config->settings().generate),
        formatError(
            "the Generator \"{}\" was not found",
            config->settings().generate));

    // --------------------------------------------------------------
    //
    // Load the compilation database file
    //
    // --------------------------------------------------------------
    std::string inputPath;
    if (toolArgs.inputPaths.empty())
    {
        inputPath = getCurrentWorkingDirectory();       
    }
    else
    {
        MRDOCS_CHECK(toolArgs.inputPaths.size() == 1,
            formatError(
                "got {} input paths where 1 was expected",
                toolArgs.inputPaths.size()));

        inputPath = toolArgs.inputPaths.front();
    }
    auto const res = generateCompilationDatabaseIfNeeded(inputPath);
    if ( ! res)
    {
        report::error("Failed to generate compilation database");
        return {};
    } 
    inputPath = *res;  

    auto compilationsPath = files::normalizePath(inputPath);
    MRDOCS_TRY(compilationsPath, files::makeAbsolute(compilationsPath));
    std::string errorMessage;
    MRDOCS_TRY_MSG(
        auto& compileCommands,
        tooling::JSONCompilationDatabase::loadFromFile(
            compilationsPath,
            errorMessage,
            tooling::JSONCommandLineSyntax::AutoDetect),
        std::move(errorMessage));

    // Get the default include paths for each compiler
    auto const defaultIncludePaths = getCompilersDefaultIncludeDir(compileCommands);

    // Custom compilation database that converts relative paths to absolute
    auto compileCommandsDir = files::getParentDir(compilationsPath);
    MrDocsCompilationDatabase compilationDatabase(
            compileCommandsDir, compileCommands, config, defaultIncludePaths);

    // Normalize outputPath path
    MRDOCS_CHECK(toolArgs.outputPath, "The output path argument is missing");
    toolArgs.outputPath = files::normalizePath(
        files::makeAbsolute(toolArgs.outputPath,
            (*config)->workingDir));

    // --------------------------------------------------------------
    //
    // Build corpus
    //
    // --------------------------------------------------------------
    MRDOCS_TRY(
        auto corpus,
        CorpusImpl::build(
            report::Level::info, config, compilationDatabase));

    if(corpus->empty())
    {
        report::warn("Corpus is empty, not generating docs");
        return {};
    }

    // --------------------------------------------------------------
    //
    // Generate docs
    //
    // --------------------------------------------------------------
    report::info("Generating docs\n");
    MRDOCS_TRY(generator.build(toolArgs.outputPath.getValue(), *corpus));
    return {};
}

} // mrdocs
} // clang
