//
// This is a derivative work. originally part of the LLVM Project.
// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// Copyright (c) 2024 Fernando Pelliccioni (fpelliccioni@gmail.com)
//
// Official repository: https://github.com/cppalliance/mrdocs
//

#include "../Gen/xml/CXXTags.hpp"
#include "TagfileWriter.hpp"
#include "lib/Lib/ConfigImpl.hpp"

namespace clang {
namespace mrdocs {

//------------------------------------------------
//
// TagfileWriter
//
//------------------------------------------------

TagfileWriter::
TagfileWriter(
    llvm::raw_ostream& os,
    Corpus const& corpus) noexcept
    : tags_(os)
    , os_(os)
    , corpus_(corpus)
{
}

void 
TagfileWriter::
initialize() {
    os_ << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    os_ << "<tagfile>\n";
}

void 
TagfileWriter::
finalize() {
    os_ << "</tagfile>\n";
}

//------------------------------------------------

void
TagfileWriter::
writeIndex()
{
    // Do nothing
}

//------------------------------------------------

template<class T>
void
TagfileWriter::
operator()(
    T const& I
    , std::string_view filename
    )
{
    #define INFO_PASCAL(Type) if constexpr(T::is##Type()) write##Type(I, filename);
    #include <mrdocs/Metadata/InfoNodes.inc>
}

template<class T>
void
TagfileWriter::
operator()(
    T const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    #define INFO_PASCAL(Type) if constexpr(T::is##Type()) write##Type(I, filename, SimpleWriterTag{});
    #include <mrdocs/Metadata/InfoNodes.inc>
}

//------------------------------------------------

void
TagfileWriter::
writeNamespace(
    NamespaceInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    // Do nothing
}

bool 
TagfileWriter::
containsOnlyNamespaces(
    NamespaceInfo const& I) const
{
    bool onlyNamespaces = true;
    corpus_.traverse(I, [&](Info const& I)
    {
        if (I.Kind != InfoKind::Namespace)
        {
            onlyNamespaces = false;
            return false;
        }
        return true;
    });
    return onlyNamespaces;
}

void
TagfileWriter::
writeNamespace(
    NamespaceInfo const& I,
    std::string_view filename)
{
    if (!containsOnlyNamespaces(I))
    {
        tags_.open("compound", {
            { "kind", "namespace" }
        });

        std::string temp;
        temp.reserve(256);
        tags_.write("name", corpus_.getFullyQualifiedName(I, temp));
        tags_.write("name", I.Name);

        corpus_.traverseIf(I, [](Info const& I)
        {
            return I.Kind != InfoKind::Function;
        }, *this, filename, SimpleWriterTag{});

        corpus_.traverseIf(I, [](Info const& I)
        {
            return I.Kind == InfoKind::Function;
        }, *this, filename, SimpleWriterTag{});    

        tags_.close("compound");
    }
}

template<class T>
void
TagfileWriter::
writeClassLike(
    T const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    std::string temp;
    temp.reserve(256);    
    tags_.write("class", corpus_.getFullyQualifiedName(I, temp), {{"kind", "class"}});    
}

template<class T>
void
TagfileWriter::
writeClassLike(
    T const& I,
    std::string_view filename)
{
    tags_.open("compound", {
        { "kind", "class" }
    });

    std::string temp;
    temp.reserve(256);
    tags_.write("name", corpus_.getFullyQualifiedName(I, temp));
    tags_.write("filename", filename);
    tags_.close("compound");  
}

void
TagfileWriter::
writeEnum(
    EnumInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeEnum(
    EnumInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeEnumerator(
    EnumeratorInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeEnumerator(
    EnumeratorInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeFriend(
    FriendInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeFriend(
    FriendInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeFunction(
    FunctionInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    // Do nothing
}

void
TagfileWriter::
writeFunction(
    FunctionInfo const& I,
    std::string_view filename)
{
    using xml::toString;

    tags_.open("member", {{"kind", "function"}});
    tags_.write("type", toString(*I.ReturnType));
    tags_.write("name", I.Name);
    
    std::string arglist = "(";
    for(auto const& J : I.Params)
    {
        arglist += toString(*J.Type);
        arglist += " ";
        arglist += J.Name;
        arglist += ", ";
    }
    if (arglist.size() > 2) {
        arglist.resize(arglist.size() - 2);
    }    
    arglist += ")";
    tags_.write("arglist", arglist);
    tags_.write("anchorfile", filename);  // Utilizamos `filename`
    tags_.write("anchor", "");
    tags_.close("member");
}

void
TagfileWriter::
writeGuide(
    GuideInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    // Do nothing
}

void
TagfileWriter::
writeGuide(
    GuideInfo const& I,
    std::string_view filename)
{
}

void
TagfileWriter::
writeConcept(
    ConceptInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeConcept(
    ConceptInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}


void
TagfileWriter::
writeAlias(
    AliasInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeAlias(
    AliasInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeUsing(
    UsingInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeUsing(
    UsingInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeRecord(
    RecordInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeRecord(
    RecordInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeTypedef(
    TypedefInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    writeClassLike(I, filename, SimpleWriterTag{});
}

void
TagfileWriter::
writeTypedef(
    TypedefInfo const& I,
    std::string_view filename)
{
    writeClassLike(I, filename);
}

void
TagfileWriter::
writeField(
    const FieldInfo& I,
    std::string_view filename,
    SimpleWriterTag)
{
    // Do nothing
}

void
TagfileWriter::
writeField(
    const FieldInfo& I,
    std::string_view filename)
{
    // Do nothing
}

void
TagfileWriter::
writeVariable(
    VariableInfo const& I,
    std::string_view filename,
    SimpleWriterTag)
{
    // Do nothing
}

void
TagfileWriter::
writeVariable(
    VariableInfo const& I,
    std::string_view filename)
{
    // Do nothing
}

void
TagfileWriter::
openTemplate(
    std::unique_ptr<TemplateInfo> const& I)
{
    // Do nothing
}

void
TagfileWriter::
closeTemplate(
    std::unique_ptr<TemplateInfo> const& I)
{
    // Do nothing
}

void
TagfileWriter::
writeSpecialization(
    const SpecializationInfo& I,
    std::string_view filename,
    SimpleWriterTag)
{
    // Do nothing
}

void
TagfileWriter::
writeSpecialization(
    const SpecializationInfo& I,
    std::string_view filename)
{
    // Do nothing
}


#define DEFINE(T) template void \
    TagfileWriter::operator()<T>(T const&, std::string_view)

#define INFO_PASCAL(Type) DEFINE(Type##Info);
#include <mrdocs/Metadata/InfoNodes.inc>

} // mrdocs
} // clang
