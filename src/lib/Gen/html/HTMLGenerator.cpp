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

#include "HTMLCorpus.hpp"
#include "HTMLGenerator.hpp"
#include "Builder.hpp"
#include "MultiPageVisitor.hpp"
#include "SinglePageVisitor.hpp"
#include "lib/Support/SafeNames.hpp"
#include <mrdocs/Metadata/DomMetadata.hpp>
#include <mrdocs/Support/Error.hpp>
#include <mrdocs/Support/Path.hpp>
#include <optional>
#include <vector>

namespace clang {
namespace mrdocs {
namespace html {

Expected<ExecutorGroup<Builder>>
createExecutors(
    DomCorpus const& domCorpus)
{
    MRDOCS_TRY(auto options, loadOptions(*domCorpus));
    auto const& config = domCorpus->config;
    auto& threadPool = config.threadPool();
    ExecutorGroup<Builder> group(threadPool);
    for(auto i = threadPool.getThreadCount(); i--;)
    {
        try
        {
           group.emplace(domCorpus, options);
        }
        catch(Exception const& ex)
        {
            return Unexpected(ex.error());
        }
    }
    return group;
}

//------------------------------------------------
//
// HTMLGenerator
//
//------------------------------------------------

Error
HTMLGenerator::
build(
    std::string_view outputPath,
    Corpus const& corpus) const
{
    if(! corpus.config->multiPage)
        return Generator::build(outputPath, corpus);

    HTMLCorpus domCorpus(corpus);
    auto ex = createExecutors(domCorpus);
    if(! ex)
        return ex.error();

    MultiPageVisitor visitor(*ex, outputPath, corpus);
    visitor(corpus.globalNamespace());
    auto errors = ex->wait();
    if(! errors.empty())
        return Error(errors);
    return Error::success();
}

Error
HTMLGenerator::
buildOne(
    std::ostream& os,
    Corpus const& corpus) const
{
    HTMLCorpus domCorpus(corpus);
    auto ex = createExecutors(domCorpus);
    if(! ex)
        return ex.error();

    std::vector<Error> errors;

    ex->async(
        [&os](Builder& builder)
        {
            auto pageText = builder.renderSinglePageHeader().value();
            os.write(pageText.data(), pageText.size());
        });
    report::info("HTMLGenerator::buildOne() - before wait header");
    errors = ex->wait();
    report::info("HTMLGenerator::buildOne() - after wait header");
    if(! errors.empty()) {
        return Error(errors);
    }

    SinglePageVisitor visitor(*ex, corpus, os);
    visitor(corpus.globalNamespace());
    report::info("HTMLGenerator::buildOne() - before wait body");
    errors = ex->wait();
    report::info("HTMLGenerator::buildOne() - after wait body");
    if(! errors.empty())
        return Error(errors);

    ex->async(
        [&os](Builder& builder)
        {
            auto pageText = builder.renderSinglePageFooter().value();
            os.write(pageText.data(), pageText.size());
        });
    report::info("HTMLGenerator::buildOne() - before wait footer");
    errors = ex->wait();
    report::info("HTMLGenerator::buildOne() - after wait footer");
    if(! errors.empty()) {
        return Error(errors);
    }

    return Error::success();
}

} // html

//------------------------------------------------

std::unique_ptr<Generator>
makeHTMLGenerator()
{
    return std::make_unique<html::HTMLGenerator>();
}

} // mrdocs
} // clang
