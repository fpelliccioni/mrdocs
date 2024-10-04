//
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2023 Vinnie Falco (vinnie.falco@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdocs
//

#ifndef MRDOCS_LIB_GEN_ADOC_SINGLEPAGEVISITOR_HPP
#define MRDOCS_LIB_GEN_ADOC_SINGLEPAGEVISITOR_HPP

#include "Builder.hpp"
#include <mrdocs/MetadataFwd.hpp>
#include <lib/Lib/TagfileWriter.hpp>
#include <mrdocs/Support/ExecutorGroup.hpp>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

namespace clang {
namespace mrdocs {
namespace adoc {

/** Visitor which writes everything to a single page.
*/
class SinglePageVisitor
{
    ExecutorGroup<Builder>& ex_;
    Corpus const& corpus_;
    std::ostream& os_;
    std::size_t numPages_ = 0;
    std::mutex mutex_;
    std::size_t topPage_ = 0;
    std::vector<std::optional<
        std::string>> pages_;
    std::string fileName_;
    TagfileWriter& tagfileWriter_;

    void writePage(std::string pageText, std::size_t pageNumber);
public:
    SinglePageVisitor(
        ExecutorGroup<Builder>& ex,
        Corpus const& corpus,
        std::ostream& os,
        std::string_view fileName,
        TagfileWriter& tagfileWriter) noexcept
        : ex_(ex)
        , corpus_(corpus)
        , os_(os)
        , fileName_(fileName)
        , tagfileWriter_(tagfileWriter)
    {
    }

    template<class T>
    void operator()(T const& I);
    void operator()(OverloadSet const& OS);

};

} // adoc
} // mrdocs
} // clang

#endif
