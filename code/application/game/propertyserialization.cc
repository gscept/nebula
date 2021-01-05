//------------------------------------------------------------------------------
//  propertyserialization.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "propertyserialization.h"
#include "memdb/typeregistry.h"
#include "game/entity.h"
#include "graphics/graphicsentity.h"
#include "memdb/propertyid.h"

namespace Game
{

PropertySerialization* PropertySerialization::Singleton = nullptr;

//------------------------------------------------------------------------------
/**
    The registry's constructor is called by the Instance() method, and
    nobody else.
*/
PropertySerialization*
PropertySerialization::Instance()
{
    if (nullptr == Singleton)
    {
        Singleton = n_new(PropertySerialization);
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
PropertySerialization::Destroy()
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
PropertySerialization::Deserialize(Ptr<IO::JsonReader> const& reader, PropertyId pid, void* ptr)
{
    n_assert(Singleton != nullptr);
    Singleton->serializers[pid.id].deserializeJson(reader, 0, ptr);
}

//------------------------------------------------------------------------------
/**
*/
void
PropertySerialization::Serialize(Ptr<IO::JsonWriter> const& writer, PropertyId pid, void* ptr)
{
    n_assert(Singleton != nullptr);
    const char* name = MemDb::TypeRegistry::GetDescription(pid)->name.Value();
    Singleton->serializers[pid.id].serializeJson(writer, name, ptr);
}

//------------------------------------------------------------------------------
/**
*/
PropertySerialization::PropertySerialization()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
PropertySerialization::~PropertySerialization()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
PropertySerialization::ValidateTypeSize(MemDb::PropertyId pid, uint32_t size)
{
    if (!MemDb::TypeRegistry::TypeSize(pid) == size)
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
    this->Get(entity.id, key);
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonWriter::Add<Game::Entity>(Game::Entity const& entity, Util::String const& key)
{
    this->Add(entity.id, key);
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Graphics::GraphicsEntityId>(Graphics::GraphicsEntityId& entity, const char* key)
{
    this->Get(entity.id, key);
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonWriter::Add<Graphics::GraphicsEntityId>(Graphics::GraphicsEntityId const& entity, Util::String const& key)
{
    this->Add(entity.id, key);
}
