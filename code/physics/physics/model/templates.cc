//------------------------------------------------------------------------------
//  physicscommon.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "util/fourcc.h"
#include "physics/collider.h"
#include "templates.h"

namespace Physics
{

PhysicsCommon::PhysicsCommon():
	friction(-1.0f), //< -1.0f = use default
	restitution(-1.0f),
	category(CollideCategory::Default),
	material(Physics::InvalidMaterial)
{
		// empty
}

PhysicsCommon::PhysicsCommon(Util::FourCC f, Util::String iname, const Math::matrix44 & startTrans, const Ptr<Collider> & inCollider):
	type(f), 
	name(iname),
	startTransform(startTrans),
	collider(inCollider),
	mass(0),
	bodyFlags(0),
	friction(-1.0f), //< -1.0f = use default
	restitution(-1.0f),
	category(CollideCategory::Default),
	material(Physics::InvalidMaterial)
{
	// empty
}

}