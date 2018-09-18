#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseProbe
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/

#include "physics/staticobject.h"
#include "physics/collider.h"
#include "math/matrix44.h"


namespace Physics
{

class Scene;

class BaseProbe : public StaticObject
{
	__DeclareClass(BaseProbe);

public:
	BaseProbe(){}
	~BaseProbe(){}

	virtual Util::Array<Ptr<Core::RefCounted>> GetOverlappingObjects() = 0;
	/// convenience function
	void Init(const Ptr<Collider> & coll, const Math::matrix44 & trans);


protected:	

};
}