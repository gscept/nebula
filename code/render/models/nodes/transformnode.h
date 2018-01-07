#pragma once
//------------------------------------------------------------------------------
/**
	The transform node contains just a hierarchical transform
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/matrix44.h"
#include "math/transform44.h"
#include "math/quaternion.h"
namespace Models
{
class TransformNode : public ModelNode
{
public:
	/// constructor
	TransformNode();
	/// destructor
	virtual ~TransformNode();

protected:
	friend class StreamModelPool;

	/// load transform
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	struct Instance : public ModelNode::Instance
	{
		Math::transform44 transform;
		Math::matrix44 modelTransform;
		bool isInViewSpace;
		bool lockedToViewer;
	};

	Math::point position;
	Math::quaternion rotate;
	Math::vector scale;
	Math::point rotatePivot;
	Math::point scalePivot;
	bool isInViewSpace;
	float minDistance;
	float maxDistance;
	bool useLodDistances;
	bool lockedToViewer;
};
} // namespace Models