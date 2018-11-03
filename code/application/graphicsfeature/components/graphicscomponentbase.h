// NIDL #version:20#
#pragma once
//------------------------------------------------------------------------------
/**
    This file was generated with Nebula's IDL compiler tool.
    DO NOT EDIT
*/
#include "game/component/component.h"
#include "game/attr/attrid.h"

//------------------------------------------------------------------------------
namespace Attr
{
    DeclareUInt(GraphicsEntity, 'gEnt', Attr::ReadOnly);
} // namespace Attr

//------------------------------------------------------------------------------
namespace GraphicsFeature
{

class GraphicsComponentBase : public Game::Component<
    decltype(Attr::GraphicsEntity)
>
{
    __DeclareClass(GraphicsComponentBase)

public:
    /// Default constructor
    GraphicsComponentBase();
    /// Default destructor
    ~GraphicsComponentBase();

    enum AttributeIndex
    {
        OWNER,
        GRAPHICSENTITY,

        NumAttributes
    };

    /// Registers an entity to this component.
    uint32_t RegisterEntity(const Game::Entity& entity);

    /// Deregister Entity.
    void DeregisterEntity(const Game::Entity& entity);

    /// Destroys all instances of this component, and deregisters every entity.
    void DestroyAll();

    /// Optimize data array and pack data
    SizeT Optimize();

    /// Called from entitymanager if this component is registered with a deletion callback.
    /// Removes entity immediately from component instances.
    void OnEntityDeleted(Game::Entity entity);
};
        
} // namespace GraphicsFeature
