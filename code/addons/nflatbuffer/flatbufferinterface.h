#pragma once
//------------------------------------------------------------------------------
/**
    Interface for dealing with flatbuffer files

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/

#include "io/uri.h"
#include "util/stringatom.h"
#include "flatbuffers/flatbuffers.h"
#include "nflatbuffer/nebula_flat.h"
#include "util/blob.h"
#include "io/ioserver.h"
#include "flatbuffers/idl.h"

#define SerializeFlatbufferText(TYPE, ITEM) Flat::FlatbufferInterface::SerializeHelper<TYPE>(ITEM, TYPE##Identifier())
#define SerializeFlatbufferTextDirect(TYPE, BUFFER) Flat::FlatbufferInterface::BufferToText(BUFFER, TYPE##Identifier())
#define CompileFlatbuffer(TYPE, SOURCE, TARGET) Flat::FlatbufferInterface::Compile(SOURCE, TARGET, TYPE##Identifier())

namespace Flat
{

class FlatbufferInterface
{
public:

    /// Initialize datastructures and load all schema files in export
    static void Init();
    /// 
    static bool LoadSchema(IO::URI const& file);
    ///
    static bool CompileSchema(IO::URI const& file, IO::URI const& outFile);
    ///
    static Util::Blob ParseJson(IO::URI const& file);
    ///
    static Util::Blob ParseJson(IO::URI const& file, const Util::String& schema);
    ///
    static bool HasSchema(Util::StringAtom identifier);

    /// Helper function, use SerializeFlatbuffer macro instead
    template<typename BaseT, typename ItemT> static Util::String SerializeHelper(ItemT const& item, const char* ident);

    /// Serialize to binary blob
    template<typename BaseT, typename ItemT> static Util::Blob SerializeFlatbuffer(ItemT const& item);

    ///
    template<typename BaseT, typename ItemT> static void DeserializeFlatbufferFile(ItemT& item, IO::URI const& file);
    ///
    template<typename BaseT, typename ItemT> static void DeserializeFlatbuffer(ItemT& item, const uint8_t* buf);

    ///
    template<typename BaseT, typename ItemT> static bool DeserializeJsonFlatbuffer(ItemT& item, const IO::URI& file, const Util::String& rootName);

    ///
    static Util::String BufferToText(const uint8_t* buffer, Util::StringAtom identifier);

    /// compile flatbuffer json to binary
    static bool Compile(IO::URI const& source, IO::URI const& targetFolder, const char* ident);

private:
    static flatbuffers::Parser* CreateParserForJson(IO::URI const& file);
};


//------------------------------------------------------------------------------
/**
*/
template<typename BaseT, typename ItemT> Util::String
FlatbufferInterface::SerializeHelper(ItemT const& item, const char* ident)
{
    Util::StringAtom sIdent(ident);
    n_assert(FlatbufferInterface::HasSchema(sIdent));
    flatbuffers::FlatBufferBuilder builder(65536);
    builder.Finish(BaseT::Pack(builder, &item));
    return FlatbufferInterface::BufferToText(builder.GetBufferPointer(), sIdent);
}

//------------------------------------------------------------------------------
/**
*/
template <typename BaseT, typename ItemT> Util::Blob
FlatbufferInterface::SerializeFlatbuffer(ItemT const& item)
{
    flatbuffers::FlatBufferBuilder builder(65536);
    builder.Finish(BaseT::Pack(builder, &item));
    return Util::Blob(builder.GetBufferPointer(), builder.GetSize());
}

//------------------------------------------------------------------------------
/**
*/
template<typename BaseT, typename ItemT> 
void
FlatbufferInterface::DeserializeFlatbuffer(ItemT& item, const uint8_t* buf)
{
    const BaseT* bItem = flatbuffers::GetRoot<BaseT>(buf);
    bItem->UnPackTo(&item);
}

//------------------------------------------------------------------------------
/**
*/
template<typename BaseT, typename ItemT>
void
FlatbufferInterface::DeserializeFlatbufferFile(ItemT& item, IO::URI const& file)
{
    Util::String contents;
    if (IO::IoServer::Instance()->ReadFile(file, contents))
    {
        const BaseT* bItem = flatbuffers::GetRoot<BaseT>(contents.AsCharPtr());
        bItem->UnPackTo(&item);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<typename BaseT, typename ItemT> 
bool 
FlatbufferInterface::DeserializeJsonFlatbuffer(ItemT& item, const IO::URI& file, const Util::String& rootName)
{
    Util::Blob flatBuffer = FlatbufferInterface::ParseJson(file, rootName);
    if (flatBuffer.IsValid())
    {
        const BaseT* bItem = flatbuffers::GetRoot<BaseT>(flatBuffer.GetPtr());
        bItem->UnPackTo(&item);
        return true;
    }
    return false;
}
}
