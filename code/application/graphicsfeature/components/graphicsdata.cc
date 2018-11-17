// NIDL #version:39#
//------------------------------------------------------------------------------
//  graphicsdata.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsdata.h"
//------------------------------------------------------------------------------
namespace Attr
{
    DefineUIntWithDefault(GraphicsEntity, 'gEnt', Attr::ReadOnly, uint(-1));
    DefineStringWithDefault(ModelResource, 'mdlR', Attr::ReadOnly, Util::String("mdl:Buildings/castle_tower.n3"));
} // namespace Attr
//------------------------------------------------------------------------------
namespace GraphicsFeature
{

__ImplementWeakClass(GraphicsFeature::GraphicsComponentData, 'GFXC', Game::ComponentInterface);
__RegisterClass(GraphicsComponentData)

//------------------------------------------------------------------------------
/**
*/
GraphicsComponentData::GraphicsComponentData() :
    component_templated_t({
        Attr::GraphicsEntity,
        Attr::ModelResource,
    })
{
    this->events.SetBit(Game::ComponentEvent::OnActivate);
    this->events.SetBit(Game::ComponentEvent::OnDeactivate);
    
}

//------------------------------------------------------------------------------
/**
*/
GraphicsComponentData::~GraphicsComponentData()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
GraphicsComponentData::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
    
    this->functions.OnActivate(instance);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponentData::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->functions.OnDeactivate(index);
        
        this->DeregisterEntityImmediate(entity);
        Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);
        return;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponentData::DestroyAll()
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
GraphicsComponentData::Optimize()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponentData::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        return;
    }
}

} // namespace GraphicsFeature
