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

#include "lib/Lib/CMakeExecution.hpp"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>

namespace clang {
namespace mrdocs {

// Expected<std::string>
// getCmakePath() {
//     std::vector<std::string> paths = {
//         "/usr/bin/cmake",
//         "/usr/local/bin/cmake",
//         "/opt/homebrew/bin/cmake",
//         "/opt/homebrew/opt/cmake/bin/cmake",
//         "/usr/local/opt/cmake/bin/cmake",
//         "/usr/local/Cellar/cmake/*/bin/cmake",
//         "/usr/local/Cellar/cmake@*/*/bin/cmake",
//         "C:/Program Files/CMake/bin/cmake.exe",
//         "C:/Program Files (x86)/CMake/bin/cmake.exe"
//     };

//     for (auto const& path : paths) {
//         if (llvm::sys::fs::exists(path)) {
//             std::optional<llvm::StringRef> const redirects[] = {llvm::StringRef(), llvm::StringRef(), llvm::StringRef()};
//             std::vector<llvm::StringRef> const args = {path, "--version"};
//             int const result = llvm::sys::ExecuteAndWait(path, args, std::nullopt, redirects);
//             if (result != 0) 
//             {
//                 return Unexpected(Error("cmake execution failed"));
//             }
//             return path;
//         }
//     }
//     return Unexpected(Error("cmake executable not found"));
// }

Expected<std::string>
getCmakePath() {

    // ErrorOr< std::string > llvm::sys::findProgramByName	(	StringRef 	Name,
    // ArrayRef< StringRef > 	Paths = {} )	

    std::vector<std::string> paths = {
        "/usr/bin/cmake",
        "/usr/local/bin/cmake",
        "/opt/homebrew/bin/cmake",
        "/opt/homebrew/opt/cmake/bin/cmake",
        "/usr/local/opt/cmake/bin/cmake",
        "/usr/local/Cellar/cmake/*/bin/cmake",
        "/usr/local/Cellar/cmake@*/*/bin/cmake",
        "C:/Program Files/CMake/bin/cmake.exe",
        "C:/Program Files (x86)/CMake/bin/cmake.exe"
    };

    auto const path = llvm::sys::findProgramByName("cmake", paths);
    if (! path) {
        return Unexpected(Error("cmake executable not found"));
    }

    std::optional<llvm::StringRef> const redirects[] = {llvm::StringRef(), llvm::StringRef(), llvm::StringRef()};
    std::vector<llvm::StringRef> const args = {*path, "--version"};
    int const result = llvm::sys::ExecuteAndWait(*path, args, std::nullopt, redirects);
    if (result != 0) 
    {
        return Unexpected(Error("cmake execution failed"));
    }
    return path;
}


Expected<std::string>
executeCmakeExportCompileCommands(llvm::StringRef cmakeListsPath) 
{
    auto cmakePathRes = getCmakePath();
    if (! cmakePathRes) 
    {
        return cmakePathRes;
    }
    auto cmakePath = *cmakePathRes;

    if ( ! llvm::sys::fs::exists(cmakeListsPath)) 
    {
        return Unexpected(Error("CMakeLists.txt not found"));
    }

    llvm::SmallString<128> tempDir;
    if (auto ec = llvm::sys::fs::createUniqueDirectory("compile_commands", tempDir)) 
    {
        return Unexpected(Error("Failed to create temporary directory"));
    }

    std::vector<llvm::StringRef> const args = {cmakePath, "-S", cmakeListsPath.str(), "-B", tempDir.str(), "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"};
    int const result = llvm::sys::ExecuteAndWait(cmakePath, args);
    if (result != 0) 
    {
        return Unexpected(Error("CMake execution failed"));
    }

    llvm::SmallString<128> compileCommandsPath(tempDir);
    llvm::sys::path::append(compileCommandsPath, "compile_commands.json");

    return compileCommandsPath.str().str();
}

} // mrdocs
} // clang
