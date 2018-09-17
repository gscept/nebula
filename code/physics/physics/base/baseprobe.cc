//------------------------------------------------------------------------------
//  baseprobe.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/base/baseprobe.h"
#include "physics/collider.h"

namespace Physics
{
	__ImplementAbstractClass(Physics::BaseProbe, 'PBPR', Physics::StaticObject);

//------------------------------------------------------------------------------
/**
*/
void 
BaseProbe::Init(const Ptr<Collider> & coll, const Math::matrix44 & trans)
{
	this->common.collider = coll;
	this->common.startTransform = trans;
}

}