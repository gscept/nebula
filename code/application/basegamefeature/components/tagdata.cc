// NIDL #version:48#
#ifdef _WIN32
#define NOMINMAX
#endif
//------------------------------------------------------------------------------
//  tagdata.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "tagdata.h"
//------------------------------------------------------------------------------
namespace Attr
{
    DefineGuid(Tag, 'TAG!', Attr::ReadWrite);
} // namespace Attr
//------------------------------------------------------------------------------
namespace Game
{

__ImplementWeakClass(Game::TagComponentData, 'TAGC', Game::ComponentInterface);
__RegisterClass(TagComponentData)

//------------------------------------------------------------------------------
/**
*/
TagComponentData::TagComponentData() :
    component_templated_t({
        Attr::Tag,
    })
{
}

//------------------------------------------------------------------------------
/**
*/
TagComponentData::~TagComponentData()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TagComponentData::RegisterEntity(const Game::Entity& entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
TagComponentData::DeregisterEntity(const Game::Entity& entity)
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
TagComponentData::DestroyAll()
{
    component_templated_t::DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TagComponentData::Optimize()
{
    return component_templated_t::Optimize();
}

//------------------------------------------------------------------------------
/**
*/
void
TagComponentData::OnEntityDeleted(Game::Entity entity)
{
    return;
}

} // namespace Game
