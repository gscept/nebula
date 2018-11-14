// NIDL #version:32#
//------------------------------------------------------------------------------
//  lightdata.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lightdata.h"
//------------------------------------------------------------------------------
namespace Attr
{
    DefineStringWithDefault(DebugName, 'tStr', Attr::ReadWrite, Util::String("tjene"));
    DefineFloatWithDefault(Range, 'LRAD', Attr::ReadOnly, float(10.0f));
    DefineFloat4WithDefault(Color, 'LCLR', Attr::ReadWrite, Math::float4(1.0f, 0.88f, 0.65f, 1.0f));
    DefineBoolWithDefault(CastShadows, 'SHDW', Attr::ReadWrite, bool(true));
    DefineFloatWithDefault(Angle, 'LNGL', Attr::ReadWrite, float(30.0f));
    DefineFloat4WithDefault(Direction, 'LDIR', Attr::ReadWrite, Math::float4(0.0f, 0.0f, 1.0f, 0.0f));
} // namespace Attr
//------------------------------------------------------------------------------
namespace GraphicsFeature
{

__ImplementWeakClass(GraphicsFeature::PointLightComponentData, 'plcp', Game::ComponentInterface);
__RegisterClass(PointLightComponentData)

//------------------------------------------------------------------------------
/**
*/
PointLightComponentData::PointLightComponentData() :
    component_templated_t({
        Attr::Range,
        Attr::Color,
        Attr::CastShadows,
        Attr::DebugName,
    })
{
}

//------------------------------------------------------------------------------
/**
*/
PointLightComponentData::~PointLightComponentData()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
PointLightComponentData::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentData::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        component_templated_t::DeregisterEntity(entity);
        return;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentData::DestroyAll()
{
    component_templated_t::DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
PointLightComponentData::Optimize()
{
    return component_templated_t::Optimize();
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentData::OnEntityDeleted(Game::Entity entity)
{
    return;
}


__ImplementWeakClass(GraphicsFeature::SpotLightComponentData, 'splc', Game::ComponentInterface);
__RegisterClass(SpotLightComponentData)

//------------------------------------------------------------------------------
/**
*/
SpotLightComponentData::SpotLightComponentData() :
    component_templated_t({
        Attr::Range,
        Attr::Angle,
        Attr::Direction,
        Attr::Color,
        Attr::CastShadows,
    })
{
}

//------------------------------------------------------------------------------
/**
*/
SpotLightComponentData::~SpotLightComponentData()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
SpotLightComponentData::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentData::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);
        return;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentData::DestroyAll()
{
    SizeT length = this->data.Size();
    for (SizeT i = 0; i < length; i++)
    {
        Game::EntityManager::Instance()->DeregisterDeletionCallback(this->GetOwner(i), this);
    }
    component_templated_t::DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
SpotLightComponentData::Optimize()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentData::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        return;
    }
}


__ImplementWeakClass(GraphicsFeature::DirectionalLightComponentData, 'drlc', Game::ComponentInterface);
__RegisterClass(DirectionalLightComponentData)

//------------------------------------------------------------------------------
/**
*/
DirectionalLightComponentData::DirectionalLightComponentData() :
    component_templated_t({
        Attr::Direction,
        Attr::Color,
        Attr::CastShadows,
    })
{
}

//------------------------------------------------------------------------------
/**
*/
DirectionalLightComponentData::~DirectionalLightComponentData()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
DirectionalLightComponentData::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentData::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);
        return;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentData::DestroyAll()
{
    SizeT length = this->data.Size();
    for (SizeT i = 0; i < length; i++)
    {
        Game::EntityManager::Instance()->DeregisterDeletionCallback(this->GetOwner(i), this);
    }
    component_templated_t::DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
DirectionalLightComponentData::Optimize()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentData::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        return;
    }
}

} // namespace GraphicsFeature
