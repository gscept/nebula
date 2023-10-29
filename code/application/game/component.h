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

/// Returns a component id
ComponentId GetComponentId(Util::StringAtom name);
/// Returns a component id, based on template type
template <typename COMPONENT>
ComponentId GetComponentId();

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

//------------------------------------------------------------------------------
/**
    Contains data for components flagged with COMPONENTFLAG_DECAY, that
    has been deleted this frame.
*/
struct ComponentDecayBuffer
{
    uint32_t size = 0;
    uint32_t capacity = 0;
    void* buffer = nullptr;
};

//------------------------------------------------------------------------------
/**
    Used for registering component types to the game world.

    @see    Game::World
*/
template <typename COMPONENT_TYPE>
struct ComponentRegisterInfo
{
    using OnInitFunc = void (*)(Game::World*, Game::Entity, COMPONENT_TYPE*);

    /// Set to true if the component should end up in the decay buffer before being completely destroyed.
    bool decay = false;
    /// initialization function to run for the component, or nullptr if not needed.
    OnInitFunc OnInit = nullptr;
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
