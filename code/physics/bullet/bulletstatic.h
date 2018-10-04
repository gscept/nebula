#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletStatic
	
	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "physics/base/basestatic.h"


class btCollisionObject;
class btDynamicsWorld;

namespace Bullet
{
	
class Collider;
class BulletCollider;
class Scene;
	

class BulletStatic : public Physics::BaseStatic
{

	__DeclareClass(BulletStatic);

public:
	/// constructor
	BulletStatic();
	/// destructor
	virtual ~BulletStatic();

	/// set collider category
	void SetCollideCategory(Physics::CollideCategory coll);
	/// set collision filter
	void SetCollideFilter(uint mask);
	/// update transform
	virtual void SetTransform(const Math::matrix44 & trans);
	/// set material
	void SetMaterialType(Physics::MaterialType t);
	

protected:
	friend class Scene;

	///
	void Attach(Physics::BaseScene * inWorld);
	///
	void Detach();

	btDynamicsWorld * world;
	btCollisionObject * collObj;

};

}

