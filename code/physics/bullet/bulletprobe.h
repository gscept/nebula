#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletProbe

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "physics/base/baseprobe.h"


class btPairCachingGhostObject;

namespace Bullet
{

class Scene;


class BulletProbe : public Physics::BaseProbe
{

	__DeclareClass(BulletProbe);

public:
	/// constructor
	BulletProbe();
	/// destructor
	virtual ~BulletProbe();

	Util::Array<Ptr<Core::RefCounted>> GetOverlappingObjects();

	void SetCollideCategory(Physics::CollideCategory coll);
	void SetCollideFilter(uint mask);
	virtual void SetTransform(const Math::matrix44 & trans);

protected:
	friend class Scene;
	void Attach(Physics::BaseScene * inWorld);
	void Detach();
	btPairCachingGhostObject * ghost;
	btDynamicsWorld * world;

};

}

