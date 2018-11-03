// NIDL #version:20#
//------------------------------------------------------------------------------
//  graphicscomponentbase.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicscomponentbase.h"
//------------------------------------------------------------------------------
namespace Attr
{
    DefineUInt(GraphicsEntity, 'gEnt', Attr::ReadOnly);
} // namespace Attr
//------------------------------------------------------------------------------
namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::GraphicsComponentBase, 'GFXC', Core::RefCounted)


//------------------------------------------------------------------------------
/**
*/
GraphicsComponentBase::GraphicsComponentBase() :
    component_templated_t({
        Attr::GraphicsEntity,
    })
{
    this->events.SetBit(Game::ComponentEvent::OnActivate);
    this->events.SetBit(Game::ComponentEvent::OnDeactivate);
    
}

//------------------------------------------------------------------------------
/**
*/
GraphicsComponentBase::~GraphicsComponentBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
GraphicsComponentBase::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    Game::EntityManager::Instance()->RegisterDeletionCallback(entity, this);
    
    this->OnActivate(instance);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponentBase::DeregisterEntity(const Game::Entity& entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->OnDeactivate(index);
        
        this->DeregisterEntityImmediate(entity);
        Game::EntityManager::Instance()->DeregisterDeletionCallback(entity, this);
        return;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponentBase::DestroyAll()
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
GraphicsComponentBase::Optimize()
{
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponentBase::OnEntityDeleted(Game::Entity entity)
{
    uint32_t index = this->GetInstance(entity);
    if (index != InvalidIndex)
    {
        this->DeregisterEntityImmediate(entity);
        return;
    }
}

} // namespace GraphicsFeature
