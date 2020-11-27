//------------------------------------------------------------------------------
//  propertyserialization.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "propertyserialization.h"
#include "memdb/typeregistry.h"
#include "game/entity.h"
#include "graphics/graphicsentity.h"

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
PropertySerialization::Deserialize(Ptr<IO::JsonReader> const& reader, Util::StringAtom name, void* ptr)
{
    n_assert(Singleton != nullptr);
    Singleton->serializers[name].deserializeJson(reader, 0, ptr);
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
PropertySerialization::ValidateTypeSize(Util::StringAtom name, uint32_t size)
{
    auto id = MemDb::TypeRegistry::GetPropertyId(name);
    if (!MemDb::TypeRegistry::TypeSize(id) == size)
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
IO::JsonReader::Get<Graphics::GraphicsEntityId>(Graphics::GraphicsEntityId& entity, const char* key)
{
    this->Get(entity.id, key);
}
