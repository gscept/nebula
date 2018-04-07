#pragma once
//------------------------------------------------------------------------------
/**

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "math/vector.h"
#include "util/string.h"
#include "util/fourcc.h"
#include "physics/materialtable.h"

namespace Physics
{

	enum MeshTopologyType
	{
		MeshConvex = 0,
		MeshConcave = 1,
		MeshConvexHull = 2,			
		MeshStatic = 4
	};

	enum ColliderType
	{
		ColliderSphere = 0,
		ColliderCube = 1,
		ColliderCylinder = 2,
		ColliderCapsule = 3,
		ColliderPlane = 4,
		ColliderMesh = 5
	};

	enum CollideCategory
	{
		None	= 0x00000000,
		Default = 1,
		Static = 2,
		Kinematic = 4,
		Debris = 8,
		SensorTrigger = 16,
		Characters = 32,
		Picking = 64,
		All = 0xffffffff		
	};

	/// joint types
	enum JointType
	{        
		HingeConstraint = 0,
		Hinge2Constraint,
		UniversalConstraint,
		SliderConstraint,
		Point2PointConstraint,

		NumJointTypes,
		InvalidType,
	};

	typedef struct ColliderDescription_
	{		
		Util::String name;		
		Math::matrix44 transform;
		ColliderType type;		

		/// wanted to use a union wont work with non trivial types. shame!
		struct 
		{
			MeshTopologyType meshType;
			Util::String meshResource;
			int primGroup;
		} mesh;
		struct 
		{
			float radius;
		} sphere;
		struct  
		{
			Math::vector halfWidth;
		} box;
		struct
		{
			float radius;
			float height;
		} cylinder;
		struct  
		{
			float radius;
			float height;
		} capsule;
		struct
		{
			Math::plane plane;
		} plane;

	} ColliderDescription;

	// FIXME redundant, merge with PhysicsCommon, use string database for colliders
	typedef struct PhysicsObjectDescription_
	{
		Util::FourCC type;

		Util::String name;
		CollideCategory category;
		uint collideFilterMask;
		MaterialType material; 
		Util::String colliderGroup;
		/// body specific
		float mass;	
		uint bodyFlags;	
		
	} PhysicsObjectDescription;


	typedef struct JointDescription_
	{
		Util::String name;
		Util::String body1;
		Util::String body2;
		JointType type;
		float breakingThreshold;
		// ugly, type specific amount of items of different types
		Util::Array<Math::vector> vectors;
		Util::Array<Math::matrix44> matrices;
		Util::Array<float> floats;
	} JointDescription;
	class Collider;



class PhysicsCommon
{
public:
	PhysicsCommon();
	PhysicsCommon(Util::FourCC, Util::String name, const Math::matrix44 & startTrans, const Ptr<Collider> & collider);

	Util::FourCC type;

	Util::String name;
	CollideCategory category;
	uint collideFilterMask;
	MaterialType material; 
	Ptr<Collider> collider;
	Math::matrix44 startTransform;

	/// body specific
	float mass, friction, restitution;	//< restitution = bounciness
	uint bodyFlags;
};

}