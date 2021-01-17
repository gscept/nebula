#pragma once
//------------------------------------------------------------------------------
/**
    Property serialization functions

    Implements various serialization functions for different types of properties
    
    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/memorystream.h"
#include "io/binarywriter.h"
#include "io/binaryreader.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "util/stringatom.h"
#include "util/dictionary.h"
#include "util/delegate.h"
#include "category.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
class PropertySerialization
{
public:
    using DeserializeJsonFunc = Util::Delegate<void(Ptr<IO::JsonReader> const&, const char* name, void*)>;
    using SerializeJsonFunc   = Util::Delegate<void(Ptr<IO::JsonWriter> const&, const char* name, void*)>;
    
    static PropertySerialization* Instance();
    static void Destroy();

    template<typename TYPE>
    static void Register(PropertyId pid);

    /// ptr points to the location where the value should be stored. Make sure you have room for it!
    static void Deserialize(Ptr<IO::JsonReader> const& reader, PropertyId pid, void* ptr);
    /// ptr points to the value that should be stored. Make sure you have room for it!
    static void Serialize(Ptr<IO::JsonWriter> const& writer, PropertyId pid, void* ptr);

private:
    PropertySerialization();
    ~PropertySerialization();

    static PropertySerialization* Singleton;

    /// validate that the size of MemDb::TypeRegistry's property named 'name' has typesize of 'size'
    bool ValidateTypeSize(PropertyId pid, uint32_t size);

    struct Serializer
    {
        DeserializeJsonFunc deserializeJson;
        SerializeJsonFunc serializeJson;
    };

    Util::Array<Serializer> serializers;
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
PropertySerialization::Register(PropertyId pid)
{
    // Setup a type-specific read function
    const auto read = [](Ptr<IO::JsonReader> const& reader, const char* name, void* ptr)
    {
        TYPE& value = *(static_cast<TYPE*>(ptr));
        reader->Get(value, name);
    };

    // Setup a type-specific write function
    const auto write = [](Ptr<IO::JsonWriter> const& writer, const char* name, void* ptr)
    {
        TYPE& value = *(static_cast<TYPE*>(ptr));
        writer->Add(value, name);
    };

    // TODO: Serialization

    auto* reg = Instance();
    if (!reg->ValidateTypeSize(pid, sizeof(TYPE)))
    {
        n_error("Trying to add a serializer that works on different sized property compared to what is registered in the MemDb::TypeRegistry!");
        return;
    }

    Serializer s;
    s.deserializeJson = read;
    s.serializeJson = write;
    if (reg->serializers.Size() <= pid.id)
    {
        reg->serializers.Grow();
        reg->serializers.Resize(reg->serializers.Capacity());
    }
    
    reg->serializers[pid.id] = s;
}

} // namespace Game
