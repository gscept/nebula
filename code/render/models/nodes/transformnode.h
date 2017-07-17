#pragma once
//------------------------------------------------------------------------------
/**
	The transform node contains just a hierarchical transform
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/xnamath/xna_matrix44.h"
namespace Models
{
class TransformNode : public ModelNode
{
	__DeclareClass(TransformNode);
public:
	/// constructor
	TransformNode();
	/// destructor
	virtual ~TransformNode();

protected:
	friend class ModelLoader;

	/// load transform
	virtual bool Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader);

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