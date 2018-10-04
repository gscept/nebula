//------------------------------------------------------------------------------
//  bulletjoint.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/joint.h"
#include "physics/bullet/nebulamotionstate.h"



namespace Bullet
{
	__ImplementClass(Bullet::BulletJoint, 'PBJO', Physics::BaseJoint);

using namespace Math;
using namespace Physics;

//------------------------------------------------------------------------------
/**
*/
BulletJoint::BulletJoint() : constraint(NULL)
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
BulletJoint::~BulletJoint() 
{
	if (this->IsAttached())
	{
		this->Detach();
	}
}

//------------------------------------------------------------------------------
/**
*/
bool 
BulletJoint::Setup(const Physics::JointDescription & desc)
{
#if 0
	this->common = desc;
	this->type = desc.type;

	// FIXME doesnt take into account any float parameters atm
	switch(desc.type)
	{
		case Point2PointConstraint:
		{			
			if(this->rigidBody2.isvalid())
			{
				// its a body-body joint
				this->constraint = n_new(btPoint2PointConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->body, *this->rigidBody2.cast<Bullet::BulletBody>()->body,
					Neb2BtVector(desc.vectors[0]),Neb2BtVector(desc.vectors[1])));
				
			}
			else
			{
				// its a body-point joint
				this->constraint = n_new(btPoint2PointConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->body, Neb2BtVector(desc.vectors[0])));
			}
			return true;
		}
		break;
		case HingeConstraint:
		{
			this->constraint = n_new(btHingeConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->body, *this->rigidBody2.cast<Bullet::BulletBody>()->body,
				// first pivots
				Neb2BtVector(desc.vectors[0]),Neb2BtVector(desc.vectors[1]),
				// then axes
				Neb2BtVector(desc.vectors[2]),Neb2BtVector(desc.vectors[3])));

		}
		break;
		case UniversalConstraint:
		{
			this->constraint = n_new(btUniversalConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->body, *this->rigidBody2.cast<Bullet::BulletBody>()->body,
				// first anchor
				Neb2BtVector(desc.vectors[0]),
				// then axes
				Neb2BtVector(desc.vectors[1]),Neb2BtVector(desc.vectors[2])));
		}
		break;
		case SliderConstraint:
		{
			this->constraint = n_new(btSliderConstraint (*this->rigidBody1.cast<Bullet::BulletBody>()->body, *this->rigidBody2.cast<Bullet::BulletBody>()->body,
				// frames
				Neb2BtM44Transform(desc.matrices[0]),Neb2BtM44Transform(desc.matrices[1]),true));				
		}
		break;
		default:
			this->type = InvalidType;
	}
#endif
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void BulletJoint::Attach(BaseScene* world)
{
	n_assert(this->constraint);
	n_assert(world->IsA(Bullet::BulletScene::RTTI));
	btDynamicsWorld* bworld = ((BulletScene*)world)->GetWorld();
	bworld->addConstraint(this->constraint);
	this->isAttached = true;
}

//------------------------------------------------------------------------------
/**
*/
void BulletJoint::SetEnabled(bool b)
{
	n_assert(this->constraint);
	this->constraint->setEnabled(b);
}

//------------------------------------------------------------------------------
/**
*/
bool BulletJoint::IsEnabled() const
{
	n_assert(this->constraint);
	return this->constraint->isEnabled();
}

//------------------------------------------------------------------------------
/**
    Detach the joint from the world. This will destroy the joint.
*/
void
BulletJoint::Detach()
{
    n_assert(this->IsAttached());

    // destroy joint object
    if (0 != this->constraint)
    {
		((BulletScene*)PhysicsServer::Instance()->GetScene())->GetWorld()->removeConstraint(constraint); 		
        n_delete(this->constraint);
        this->constraint = 0;
		this->isAttached = false;
    }
}

//------------------------------------------------------------------------------
/**
    This method should be overwritten in subclasses.
*/
void
BulletJoint::UpdateTransform(const Math::matrix44& m)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletJoint::SetBreakThreshold(float threshold)
{
	n_assert(this->constraint != NULL);
	this->constraint->setBreakingImpulseThreshold(threshold);
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletJoint::GetBreakThreshold()
{
	n_assert(this->constraint != NULL);
	return this->constraint->getBreakingImpulseThreshold();
}

//------------------------------------------------------------------------------
/**
*/
void
BulletJoint::RenderDebug()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletJoint::SetERP(float ERP, int axis )
{
	n_assert(this->constraint != NULL);
	this->constraint->setParam(BT_CONSTRAINT_ERP, ERP, axis);

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletJoint::SetCFM(float CFM, int axis  )
{
	n_assert(this->constraint != NULL);
	this->constraint->setParam(BT_CONSTRAINT_CFM, CFM, axis);

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletJoint::SetStoppingCFM(float CFM, int axis  )
{
	n_assert(this->constraint != NULL);
	this->constraint->setParam(BT_CONSTRAINT_STOP_CFM, CFM, axis);

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletJoint::SetStoppingERP(float ERP, int axis  )
{
	n_assert(this->constraint != NULL);
	this->constraint->setParam(BT_CONSTRAINT_STOP_ERP, ERP, axis);

}

//------------------------------------------------------------------------------
/**
*/
float
BulletJoint::GetERP(int axis  )
{
	n_assert(this->constraint != NULL);
	return this->constraint->getParam(BT_CONSTRAINT_ERP, axis);

}

//------------------------------------------------------------------------------
/**
*/
float
BulletJoint::GetCFM(int axis )
{
	n_assert(this->constraint != NULL);
	return this->constraint->getParam(BT_CONSTRAINT_CFM, axis);

}

//------------------------------------------------------------------------------
/**
*/
float
BulletJoint::GetStoppingERP(int axis )
{
	n_assert(this->constraint != NULL);
	return this->constraint->getParam(BT_CONSTRAINT_STOP_ERP, axis);

}

//------------------------------------------------------------------------------
/**
*/
float
BulletJoint::GetStoppingCFM(int axis  )
{
	n_assert(this->constraint != NULL);
	return this->constraint->getParam(BT_CONSTRAINT_STOP_CFM, axis);

}

}