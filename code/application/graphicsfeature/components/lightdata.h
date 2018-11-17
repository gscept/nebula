// NIDL #version:39#
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
    DeclareString(DebugName, 'tStr', Attr::ReadWrite);
    DeclareFloat(Range, 'LRAD', Attr::ReadOnly);
    DeclareFloat4(Color, 'LCLR', Attr::ReadWrite);
    DeclareBool(CastShadows, 'SHDW', Attr::ReadWrite);
    DeclareFloat(Angle, 'LNGL', Attr::ReadWrite);
    DeclareFloat4(Direction, 'LDIR', Attr::ReadWrite);
} // namespace Attr

//------------------------------------------------------------------------------
namespace GraphicsFeature
{

class PointLightComponentData : public Game::Component<
    decltype(Attr::Range),
    decltype(Attr::Color),
    decltype(Attr::CastShadows),
    decltype(Attr::DebugName)
>
{
    __DeclareClass(PointLightComponentData);
public:
    /// Default constructor
    PointLightComponentData();
    /// Default destructor
    ~PointLightComponentData();

    enum AttributeIndex
    {
        OWNER,
        RANGE,
        COLOR,
        CASTSHADOWS,
        DEBUGNAME,

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
        

class SpotLightComponentData : public Game::Component<
    decltype(Attr::Range),
    decltype(Attr::Angle),
    decltype(Attr::Direction),
    decltype(Attr::Color),
    decltype(Attr::CastShadows)
>
{
    __DeclareClass(SpotLightComponentData);
public:
    /// Default constructor
    SpotLightComponentData();
    /// Default destructor
    ~SpotLightComponentData();

    enum AttributeIndex
    {
        OWNER,
        RANGE,
        ANGLE,
        DIRECTION,
        COLOR,
        CASTSHADOWS,

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
        

class DirectionalLightComponentData : public Game::Component<
    decltype(Attr::Direction),
    decltype(Attr::Color),
    decltype(Attr::CastShadows)
>
{
    __DeclareClass(DirectionalLightComponentData);
public:
    /// Default constructor
    DirectionalLightComponentData();
    /// Default destructor
    ~DirectionalLightComponentData();

    enum AttributeIndex
    {
        OWNER,
        DIRECTION,
        COLOR,
        CASTSHADOWS,

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
