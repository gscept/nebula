// NIDL #version:22#
//------------------------------------------------------------------------------
//  lightcomponentbase.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lightcomponentbase.h"
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

__ImplementClass(GraphicsFeature::PointLightComponentBase, 'plcp', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
PointLightComponentBase::PointLightComponentBase() :
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
PointLightComponentBase::~PointLightComponentBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
PointLightComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::DeregisterEntity(const Game::Entity& entity)
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
PointLightComponentBase::DestroyAll()
{
    component_templated_t::DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
PointLightComponentBase::Optimize()
{
    return component_templated_t::Optimize();
}

//------------------------------------------------------------------------------
/**
*/
void
PointLightComponentBase::OnEntityDeleted(Game::Entity entity)
{
    return;
}


__ImplementClass(GraphicsFeature::SpotLightComponentBase, 'splc', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
SpotLightComponentBase::SpotLightComponentBase() :
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
SpotLightComponentBase::~SpotLightComponentBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
SpotLightComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::DeregisterEntity(const Game::Entity& entity)
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
SpotLightComponentBase::DestroyAll()
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
SpotLightComponentBase::Optimize()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
SpotLightComponentBase::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        return;
    }
}


__ImplementClass(GraphicsFeature::DirectionalLightComponentBase, 'drlc', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
DirectionalLightComponentBase::DirectionalLightComponentBase() :
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
DirectionalLightComponentBase::~DirectionalLightComponentBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
DirectionalLightComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::DeregisterEntity(const Game::Entity& entity)
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
DirectionalLightComponentBase::DestroyAll()
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
DirectionalLightComponentBase::Optimize()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
DirectionalLightComponentBase::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        return;
    }
}

} // namespace GraphicsFeature
