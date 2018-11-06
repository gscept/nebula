// NIDL #version:22#
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

class PointLightComponentBase : public Game::Component<
    decltype(Attr::Range),
    decltype(Attr::Color),
    decltype(Attr::CastShadows),
    decltype(Attr::DebugName)
>
{
    __DeclareClass(PointLightComponentBase)

public:
    /// Default constructor
    PointLightComponentBase();
    /// Default destructor
    ~PointLightComponentBase();

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
        

class SpotLightComponentBase : public Game::Component<
    decltype(Attr::Range),
    decltype(Attr::Angle),
    decltype(Attr::Direction),
    decltype(Attr::Color),
    decltype(Attr::CastShadows)
>
{
    __DeclareClass(SpotLightComponentBase)

public:
    /// Default constructor
    SpotLightComponentBase();
    /// Default destructor
    ~SpotLightComponentBase();

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
        

class DirectionalLightComponentBase : public Game::Component<
    decltype(Attr::Direction),
    decltype(Attr::Color),
    decltype(Attr::CastShadows)
>
{
    __DeclareClass(DirectionalLightComponentBase)

public:
    /// Default constructor
    DirectionalLightComponentBase();
    /// Default destructor
    ~DirectionalLightComponentBase();

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
