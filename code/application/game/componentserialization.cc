//------------------------------------------------------------------------------
//  componentserialization.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "componentserialization.h"
#include "memdb/attributeregistry.h"
#include "game/entity.h"
#include "memdb/attributeid.h"

namespace Game
{

ComponentSerialization* ComponentSerialization::Singleton = nullptr;

//------------------------------------------------------------------------------
/**
    The registry's constructor is called by the Instance() method, and
    nobody else.
*/
ComponentSerialization*
ComponentSerialization::Instance()
{
    if (nullptr == Singleton)
    {
        Singleton = new ComponentSerialization;
        n_assert(nullptr != Singleton);
    }
    return Singleton;
}

//------------------------------------------------------------------------------
/**
    This static method is used to destroy the registry object and should be
    called right before the main function exits. It will make sure that
    no accidential memory leaks are reported by the debug heap.
*/
void
ComponentSerialization::Destroy()
{
    if (nullptr != Singleton)
    {
        delete Singleton;
        Singleton = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentSerialization::Deserialize(Ptr<IO::JsonReader> const& reader, ComponentId component, void* ptr)
{
    n_assert(Singleton != nullptr);
    const char* name = MemDb::AttributeRegistry::GetAttribute(component)->name.Value();
    Singleton->serializers[component.id].deserializeJson(reader, name, ptr);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentSerialization::Serialize(Ptr<IO::JsonWriter> const& writer, ComponentId component, void* ptr)
{
    n_assert(Singleton != nullptr);
    const char* name = MemDb::AttributeRegistry::GetAttribute(component)->name.Value();
    Singleton->serializers[component.id].serializeJson(writer, name, ptr);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentSerialization::Override(ComponentId component, DeserializeJsonFunc deserialize, SerializeJsonFunc serialize)
{
    if (component.id >= Singleton->serializers.Size())
        return;

    Singleton->serializers[component.id].serializeJson = serialize;
    Singleton->serializers[component.id].deserializeJson = deserialize;
}

void
ComponentSerialization::OverrideType(OverridableType type, DeserializeJsonFunc deserialize, SerializeJsonFunc serialize)
{
    n_assert(type >= 0 && type < NUM_OVERRIDABLE_TYPES);
    Singleton->typeOverrides[type].deserializeJson = deserialize;
    Singleton->typeOverrides[type].serializeJson = serialize;
}

//------------------------------------------------------------------------------
/**
*/
ComponentSerialization::DeserializeJsonFunc&
ComponentSerialization::GetTypeDeserializeFunc(OverridableType type)
{
    n_assert(type >= 0 && type < NUM_OVERRIDABLE_TYPES);
    return Singleton->typeOverrides[type].deserializeJson;
}

//------------------------------------------------------------------------------
/**
*/
ComponentSerialization::SerializeJsonFunc&
ComponentSerialization::GetTypeSerializeFunc(OverridableType type)
{
    n_assert(type >= 0 && type < NUM_OVERRIDABLE_TYPES);
    return Singleton->typeOverrides[type].serializeJson;
}

//------------------------------------------------------------------------------
/**
*/
ComponentSerialization::ComponentSerialization()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ComponentSerialization::~ComponentSerialization()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
ComponentSerialization::ValidateTypeSize(MemDb::AttributeId component, uint32_t size)
{
    if (MemDb::AttributeRegistry::TypeSize(component) != size)
    {
        return false;
    }
    return true;
}

} // namespace Game

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Game::Entity>(Game::Entity& entity, const char* key)
{
    auto& deserialize = Game::ComponentSerialization::GetTypeDeserializeFunc(Game::ComponentSerialization::ENTITY);
    if (!deserialize)
    {
        uint64_t id;
        this->Get<uint64_t>(id, key);
        entity = Game::Entity(id);
    }
    else
    {
        deserialize(this, key, (void*)&entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonWriter::Add<Game::Entity>(Game::Entity const& entity, Util::String const& key)
{
    auto& serialize = Game::ComponentSerialization::GetTypeSerializeFunc(Game::ComponentSerialization::ENTITY);
    if (!serialize)
    {
        this->Add<uint64_t>((uint64_t)entity, key);
    }
    else
    {
        Game::Entity nonConstEntity = entity;
        serialize(this, key.AsCharPtr(), (void*)&nonConstEntity);
    }
}
