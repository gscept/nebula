#pragma once
//------------------------------------------------------------------------------
/**
    @file component.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "memdb/attributeregistry.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "game/componentserialization.h"
#include "game/componentinspection.h"

namespace Game
{

class World;

//------------------------------------------------------------------------------
/**
   Specifies special behaviour for a components
*/
enum ComponentFlags : uint32_t
{
    /// regular component
    COMPONENTFLAG_NONE = 0,
    /// Component will decay. This will delay the deletion of this component by
    /// one frame, allowing managers to clean up externally allocated resources
    COMPONENTFLAG_DECAY = 1 << 0
};

/// Returns a component id
ComponentId GetComponentId(Util::StringAtom name);
/// Returns a component id, based on template type
template <typename COMPONENT>
ComponentId GetComponentId();

using ComponentInitFunc = void (*)(Game::World*, Game::Entity, void*);

class ComponentInterface : public MemDb::Attribute
{
public:
    /// construct from template type, with default value.
    template <typename T>
    explicit ComponentInterface(Util::StringAtom name, T const& defaultValue, uint32_t flags)
        : Attribute(name, defaultValue, flags)
    {
        // empty
    }

    ComponentInitFunc Init = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
class ComponentBuilder
{
public:
    ComponentBuilder(World* world)
        : world(world)
    {
    }

    // initialization function to run for the component, or nullptr if not needed.
    ComponentBuilder&
    OnInit(void (*func)(Game::World*, Game::Entity, TYPE*))
    {
        initFunc = reinterpret_cast<ComponentInitFunc>(func);
        return *this;
    }

    // set to true if the component should end up in the decay buffer before being completely destroyed.
    ComponentBuilder& Decay(bool value)
    {
        (uint32_t&)this->componentFlags |= (uint32_t)Game::ComponentFlags::COMPONENTFLAG_DECAY * (uint32_t)value;
        return *this;
    }

    /// Builds and registers the component to the world.
    ComponentId
    Build()
    {
        n_assert(world != nullptr);

        ComponentInterface* cInterface = new ComponentInterface(TYPE::Traits::name, TYPE(), (uint32_t)this->componentFlags);
        cInterface->Init = this->initFunc;
        Game::ComponentId const cid = MemDb::AttributeRegistry::Register<TYPE>(cInterface);
        Game::ComponentSerialization::Register<TYPE>(cid);
        Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<TYPE>);
        return cid;
    }

private:
    Game::ComponentFlags componentFlags = COMPONENTFLAG_NONE;
    ComponentInitFunc initFunc = nullptr;
    Game::World* world = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
struct ComponentDecayBuffer
{
    uint32_t size = 0;
    uint32_t capacity = 0;
    void* buffer = nullptr;
};

//------------------------------------------------------------------------------
/**
    -- Template implementations --
*/
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
template <typename COMPONENT>
inline ComponentId
GetComponentId()
{
#if !PUBLIC_BUILD
    if (!MemDb::AttributeRegistry::IsRegistered<COMPONENT>())
    {
        n_error(
            "Component '%s' is not registered! Make sure you call Game::RegisterComponent<T>() for all component types before "
            "using them.",
            COMPONENT::Traits::name
        );
    }
#endif
    return MemDb::GetAttributeId<COMPONENT>();
}

//------------------------------------------------------------------------------
/**
*/
inline ComponentId
GetComponentId(Util::StringAtom name)
{
    return MemDb::AttributeRegistry::GetAttributeId(name);
}

} // namespace Game
