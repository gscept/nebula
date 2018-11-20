// NIDL #version:57#
#ifdef _WIN32
#define NOMINMAX
#endif
//------------------------------------------------------------------------------
//  transformdata.cc
//  (C) Individual contributors, see AUTHORS file
//
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformdata.h"
//------------------------------------------------------------------------------
namespace Attr
{
    DefineMatrix44(LocalTransform, 'TFLT', Attr::ReadWrite);
    DefineMatrix44(WorldTransform, 'TFWT', Attr::ReadOnly);
    DefineUIntWithDefault(Parent, 'TFPT', Attr::ReadOnly, uint(-1));
    DefineUIntWithDefault(FirstChild, 'TFFC', Attr::ReadOnly, uint(-1));
    DefineUIntWithDefault(NextSibling, 'TFNS', Attr::ReadOnly, uint(-1));
    DefineUIntWithDefault(PreviousSibling, 'TFPS', Attr::ReadOnly, uint(-1));
} // namespace Attr
//------------------------------------------------------------------------------
namespace Game
{

__ImplementWeakClass(Game::TransformComponentData, 'TFRM', Game::ComponentInterface);
__RegisterClass(TransformComponentData)

//------------------------------------------------------------------------------
/**
*/
TransformComponentData::TransformComponentData() :
    component_templated_t({
        Attr::Parent,
        Attr::FirstChild,
        Attr::NextSibling,
        Attr::PreviousSibling,
        Attr::LocalTransform,
        Attr::WorldTransform,
    })
{
}

//------------------------------------------------------------------------------
/**
*/
TransformComponentData::~TransformComponentData()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TransformComponentData::RegisterEntity(Game::Entity entity)
{
    auto instance = component_templated_t::RegisterEntity(entity);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentData::DeregisterEntity(Game::Entity entity)
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
TransformComponentData::DestroyAll()
{
    component_templated_t::DestroyAll();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TransformComponentData::Optimize()
{
    return component_templated_t::Optimize();
}

//------------------------------------------------------------------------------
/**
*/
void
TransformComponentData::OnEntityDeleted(Game::Entity entity)
{
    return;
}

//------------------------------------------------------------------------------
/**
*/
uint& TransformComponentData::Parent(uint32_t instance)
{
    return this->data.Get<1>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint& TransformComponentData::FirstChild(uint32_t instance)
{
    return this->data.Get<2>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint& TransformComponentData::NextSibling(uint32_t instance)
{
    return this->data.Get<3>(instance);
}

//------------------------------------------------------------------------------
/**
*/
uint& TransformComponentData::PreviousSibling(uint32_t instance)
{
    return this->data.Get<4>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44& TransformComponentData::LocalTransform(uint32_t instance)
{
    return this->data.Get<5>(instance);
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44& TransformComponentData::WorldTransform(uint32_t instance)
{
    return this->data.Get<6>(instance);
}

} // namespace Game
