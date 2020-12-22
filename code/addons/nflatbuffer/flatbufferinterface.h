#pragma once
//------------------------------------------------------------------------------
/**
    Interface for dealing with flatbuffer files

    (C) 2020 Individual contributors, see AUTHORS file
*/

#include "io/uri.h"
#include "util/stringatom.h"
#include "flatbuffers/flatbuffers.h"
#include "util/blob.h"

#define SerializeFlatbufferText(TYPE, ITEM) Flat::FlatbufferInterface::SerializeHelper<TYPE>(ITEM, TYPE##Identifier())
//#define Deserialize

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
    static bool HasSchema(Util::StringAtom identifier);

    /// Helper function, use SerializeFlatbuffer macro instead
    template<typename BaseT, typename ItemT> static Util::String SerializeHelper(ItemT const& item, const char* ident);

    template<typename BaseT, typename ItemT> static Util::Blob SerializeFlatbuffer(ItemT const& item);

    template<typename BaseT, typename ItemT> static void DeserializeFlatbuffer(ItemT& item, uint8_t* buf)
    {
        const BaseT* bItem = flatbuffers::GetRoot<BaseT>(buf);
        bItem->UnPackTo(&item);
    }

private:
    ///
    static Util::String BufferToText(uint8_t* buffer, Util::StringAtom identifier);

    /// internal helper function

};


//------------------------------------------------------------------------------
/**
*/
template<typename BaseT, typename ItemT> Util::String
FlatbufferInterface::SerializeHelper(ItemT const& item, const char* ident)
{
    Util::StringAtom sIdent(ident);
    n_assert(FlatbufferInterface::HasSchema(sIdent));
    flatbuffers::FlatBufferBuilder builder;
    builder.Finish(BaseT::Pack(builder, &item));
    return FlatbufferInterface::BufferToText(builder.GetBufferPointer(), sIdent);
}


template <typename BaseT, typename ItemT> Util::Blob
FlatbufferInterface::SerializeFlatbuffer(ItemT const& item)
{
    flatbuffers::FlatBufferBuilder builder;
    builder.Finish(BaseT::Pack(builder, &item));
    return Util::Blob(builder.GetBufferPointer(), builder.GetSize());
}
}