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

/// Registers a component to the component registry, serializer and inspector
template <typename T>
void RegisterComponent(ComponentFlags flags = ComponentFlags::COMPONENTFLAG_NONE);
/// Returns a component id
ComponentId GetComponentId(Util::StringAtom name);
/// Returns a component id, based on template type
template <typename COMPONENT>
ComponentId GetComponentId();

//------------------------------------------------------------------------------
/**
*/
class ComponentEvent
{
public:
    // init function to run for the component, or nullptr if not needed.
    template <typename TYPE>
    ComponentEvent&
    OnInit(void (*func)(Game::World*, Game::Entity, TYPE*))
    {
        initFunc = reinterpret_cast<GenericCallback>(func);
        return *this;
    }

    // component type that the event should be attached to
    template <typename TYPE>
    ComponentEvent&
    Type()
    {
        this->componentId = Game::GetComponentId<TYPE>();
        return *this;
    }

    // set to true if the component should end up in the decay buffer before being completely destroyed.
    ComponentEvent&
    Decay(bool value)
    {
        (uint32_t&)this->componentFlags |= (uint32_t)Game::ComponentFlags::COMPONENTFLAG_DECAY * (uint32_t)value;
        return *this;
    }

private:
    using GenericCallback = void (*)(Game::World*, Game::Entity, void*);

    Game::ComponentId componentId;
    Game::ComponentFlags componentFlags;
    GenericCallback initFunc = nullptr;

    Game::World* world;
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
template <typename T>
inline void
RegisterComponent(ComponentFlags flags)
{
    Util::StringAtom const name = T::Traits::name;
    Game::ComponentId const cid = MemDb::AttributeRegistry::Register<T>(name, T(), (uint32_t)flags);
    Game::ComponentSerialization::Register<T>(cid);
    Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<T>);
}

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
