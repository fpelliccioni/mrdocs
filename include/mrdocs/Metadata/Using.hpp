//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2024 Fernando Pelliccioni (fpelliccioni@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdocs
//

#ifndef MRDOCS_API_METADATA_USING_HPP
#define MRDOCS_API_METADATA_USING_HPP

#include <mrdocs/Platform.hpp>
#include <mrdocs/Metadata/Info.hpp>
#include <mrdocs/Metadata/Source.hpp>
#include <mrdocs/Metadata/Type.hpp>
#include <string>
#include <vector>
#include <utility>

namespace clang {
namespace mrdocs {

/** Info for using declarations and directives.
 */
struct UsingInfo
    : IsInfo<InfoKind::Using>,
    SourceInfo
{
    /** Indicates whether this is a using directive. */
    bool IsDirective = false;

    /** The symbol(s) being used.
        For declarations, this will have a single element.
        For directives, this could theoretically be empty (though unlikely in practical use).
    */
    std::vector<SymbolID> UsedSymbols;

    /** Name of the using declaration or directive.
        This could be the alias name in declarations, or the namespace name in directives.
    */
    std::string UsingName;

    //--------------------------------------------

    explicit UsingInfo(SymbolID ID, bool isDirective = false) noexcept
        : IsInfo(ID), IsDirective(isDirective)
    {
    }
};

} // mrdocs
} // clang

#endif
