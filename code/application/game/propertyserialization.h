#pragma once
//------------------------------------------------------------------------------
/**
    Property serialization functions

    Implements various serialization functions for different types of properties
    
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
#include <type_traits>

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
class PropertySerialization
{
public:
    using DeserializeJsonFunc = Util::Delegate<void(Ptr<IO::JsonReader> const&, const char* name, void*)>;
    
    static PropertySerialization* Instance();
    static void Destroy();

    template<typename TYPE>
    static void Register(Util::StringAtom name);

    /// ptr points to the location where the value should be stored. Make sure you have room for it!
    static void Deserialize(Ptr<IO::JsonReader> const& reader, Util::StringAtom name, void* ptr);

private:
    PropertySerialization();
    ~PropertySerialization();

    static PropertySerialization* Singleton;

    /// validate that the size of MemDb::TypeRegistry's property named 'name' has typesize of 'size'
    bool ValidateTypeSize(Util::StringAtom name, uint32_t size);

    struct Serializer
    {
        DeserializeJsonFunc deserializeJson;
    };

    Util::Dictionary<Util::StringAtom, Serializer> serializers;
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
PropertySerialization::Register(Util::StringAtom name)
{
    const auto foo = [](Ptr<IO::JsonReader> const& reader, const char* name, void* ptr)
    {
        TYPE& value = *(static_cast<TYPE*>(ptr));
        reader->Get(value, name);
    };

    auto* reg = Instance();

    if (!reg->ValidateTypeSize(name, sizeof(TYPE)))
    {
        n_error("Trying to add a serializer that works on different sized property compared to what is registered in the MemDb::TypeRegistry!");
        return;
    }

    Serializer s;
    s.deserializeJson = foo;
    if (!reg->serializers.Contains(name))
    {
        reg->serializers.Add(name, s);
    }
    else
    {
        n_error("Tried to register property named %s: Cannot register two properties with same name!", name.Value());
    }
}

} // namespace Game
