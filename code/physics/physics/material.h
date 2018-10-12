#pragma once
//------------------------------------------------------------------------------
/**
@class Physics2::Material

    A physics Material
    
    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics2/bullet/bulletmaterial.h"
namespace Physics2
{
class Material : public Bullet::BulletMaterial
{
    __DeclareClass(Material);      
};
}
#elif(__USE_PHYSX__)
#include "physics2/physx/physxmaterial.h"
namespace Physics2
{
class Material : public PhysX::PhysXMaterial
{
	__DeclareClass(Material);	  
};
}
#else
#error "Physics2::Material not implemented"
#endif