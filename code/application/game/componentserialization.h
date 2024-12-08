#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::ComponentSerialization
    
    Component serialization functions.

    Implements various serialization functions for different types of components
    
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
#include "game/componentid.h"
#include <functional>

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
class ComponentSerialization
{
public:
    using DeserializeJsonFunc = std::function<void(Ptr<IO::JsonReader> const&, const char* name, void*)>;
    using SerializeJsonFunc   = std::function<void(Ptr<IO::JsonWriter> const&, const char* name, void*)>;
    
    enum OverridableType
    {
        ENTITY,
        RESOURCE, // TODO: resources should be a special type, not a stringatom. Should be a wrapper around a string atom instead.
        NUM_OVERRIDABLE_TYPES
    };

    static ComponentSerialization* Instance();
    static void Destroy();

    template<typename TYPE>
    static void Register(ComponentId component);

    /// ptr points to the location where the value should be stored. Make sure you have room for it!
    static void Deserialize(Ptr<IO::JsonReader> const& reader, ComponentId component, void* ptr);
    /// ptr points to the value that should be stored
    static void Serialize(Ptr<IO::JsonWriter> const& writer, ComponentId component, void* ptr);

    /// override a specific components serialization and deserialization funcs
    static void Override(ComponentId component, DeserializeJsonFunc deserialize, SerializeJsonFunc serialize);

    /// override a specific type serializer. This will be called instead of a standard variant.
    static void OverrideType(OverridableType type, DeserializeJsonFunc deserialize, SerializeJsonFunc serialize);

    /// Get the deserialize override for a specific type.
    static DeserializeJsonFunc& GetTypeDeserializeFunc(OverridableType type);
    /// Get the serialize override for a specific type
    static SerializeJsonFunc& GetTypeSerializeFunc(OverridableType type);

private:
    ComponentSerialization();
    ~ComponentSerialization();

    static ComponentSerialization* Singleton;

    /// validate that the size of MemDb::AttributeRegistry's component named 'name' has typesize of 'size'
    bool ValidateTypeSize(ComponentId component, uint32_t size);

    struct Serializer
    {
        DeserializeJsonFunc deserializeJson;
        SerializeJsonFunc serializeJson;
    };

    Util::Array<Serializer> serializers;

    Serializer typeOverrides[NUM_OVERRIDABLE_TYPES];
};

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
ComponentSerialization::Register(ComponentId component)
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
    if (!reg->ValidateTypeSize(component, sizeof(TYPE)))
    {
        n_error("Trying to add a serializer that works on different sized component compared to what is registered in the MemDb::AttributeRegistry!");
        return;
    }

    Serializer s;
    s.deserializeJson = read;
    s.serializeJson = write;
    while (reg->serializers.Size() <= component.id)
    {
        reg->serializers.Grow();
        reg->serializers.Resize(reg->serializers.Capacity());
    }
    
    reg->serializers[component.id] = s;
}

} // namespace Game
