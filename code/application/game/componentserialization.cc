//------------------------------------------------------------------------------
//  componentserialization.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "componentserialization.h"
#include "memdb/typeregistry.h"
#include "game/entity.h"
#include "memdb/componentid.h"

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
        Singleton = n_new(ComponentSerialization);
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
        n_delete(Singleton);
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
    Singleton->serializers[component.id].deserializeJson(reader, 0, ptr);
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentSerialization::Serialize(Ptr<IO::JsonWriter> const& writer, ComponentId component, void* ptr)
{
    n_assert(Singleton != nullptr);
    const char* name = MemDb::TypeRegistry::GetDescription(component)->name.Value();
    Singleton->serializers[component.id].serializeJson(writer, name, ptr);
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
ComponentSerialization::ValidateTypeSize(MemDb::ComponentId component, uint32_t size)
{
    if (!MemDb::TypeRegistry::TypeSize(component) == size)
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
    entity = { this->GetUInt(key) };
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonWriter::Add<Game::Entity>(Game::Entity const& entity, Util::String const& key)
{
    this->Add<uint>((uint)entity, key);
}

