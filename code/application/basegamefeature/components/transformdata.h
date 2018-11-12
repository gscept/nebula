// NIDL #version:32#
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
    DeclareMatrix44(LocalTransform, 'TFLT', Attr::ReadWrite);
    DeclareMatrix44(WorldTransform, 'TFWT', Attr::ReadOnly);
    DeclareUInt(Parent, 'TFPT', Attr::ReadOnly);
    DeclareUInt(FirstChild, 'TFFC', Attr::ReadOnly);
    DeclareUInt(NextSibling, 'TFNS', Attr::ReadOnly);
    DeclareUInt(PreviousSibling, 'TFPS', Attr::ReadOnly);
} // namespace Attr

//------------------------------------------------------------------------------
namespace Game
{

class TransformComponentData : public Game::Component<
    decltype(Attr::Parent),
    decltype(Attr::FirstChild),
    decltype(Attr::NextSibling),
    decltype(Attr::PreviousSibling),
    decltype(Attr::LocalTransform),
    decltype(Attr::WorldTransform)
>
{
    __DeclareClass(TransformComponentData);
public:
    /// Default constructor
    TransformComponentData();
    /// Default destructor
    ~TransformComponentData();

    enum AttributeIndex
    {
        OWNER,
        PARENT,
        FIRSTCHILD,
        NEXTSIBLING,
        PREVIOUSSIBLING,
        LOCALTRANSFORM,
        WORLDTRANSFORM,

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
        
} // namespace Game
