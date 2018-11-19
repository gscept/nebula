// NIDL #version:51#
#pragma once
//------------------------------------------------------------------------------
/**
    This file was generated with Nebula's IDL compiler tool.
    DO NOT EDIT
*/
#include "game/entity.h"
#include "game/component/component.h"
#include "game/attr/attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
    DeclareGuid(Tag, 'TAG!', Attr::ReadWrite);
} // namespace Attr

//------------------------------------------------------------------------------
namespace Game
{

class TagComponentData : public Game::Component<
    decltype(Attr::Tag)
>
{
    __DeclareClass(TagComponentData);
public:
    /// Default constructor
    TagComponentData();
    /// Default destructor
    ~TagComponentData();

    enum AttributeIndex
    {
        OWNER,
        TAG,

        NumAttributes
    };

    /// Registers an entity to this component.
    uint32_t RegisterEntity(Game::Entity entity);

    /// Deregister Entity.
    void DeregisterEntity(Game::Entity entity);

    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();

    /// Optimize data array and pack data
    SizeT Optimize();

    /// Called from entitymanager if this component is registered with a deletion callback.
    /// Removes entity immediately from component instances.
    void OnEntityDeleted(Game::Entity entity);
};
        
} // namespace Game
