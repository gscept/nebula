#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BasePointToPoint

    A joint (also known as constraint) connects two Actors    

    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joint.h"

//------------------------------------------------------------------------------
namespace Physics
{

class BasePointToPoint : public Joint
{
    __DeclareAbstractClass(BasePointToPoint);
public:
    

    /// default constructor
	BasePointToPoint() {}    

	virtual void Setup(const Math::vector & pivot) = 0;
	virtual void Setup(const Math::vector & pivotA, const Math::vector & pivotB) = 0;
	
	virtual void SetPivotA(const Math::vector & pivot) = 0;
	virtual void SetPivotB(const Math::vector & pivot) = 0;

	virtual Math::vector GetPivotA() = 0;
	virtual Math::vector GetPivotB() = 0;

	virtual void SetJointParams(float tau, float damping, float impulseclamp) = 0;

};
}
