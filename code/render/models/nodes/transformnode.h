#pragma once
//------------------------------------------------------------------------------
/**
	The transform node contains just a hierarchical transform
	
	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/mat4.h"
#include "math/transform44.h"
#include "math/quat.h"
namespace Models
{
class TransformNode : public ModelNode
{
public:
	/// constructor
	TransformNode();
	/// destructor
	virtual ~TransformNode();

	struct Instance : public ModelNode::Instance
	{
		Math::transform44 transform;
		Math::mat4 modelTransform;
		Math::mat4 invModelTransform;
		
		/// Identifiable object id. Usually the graphics entity id.
		/// @todo	Should be moved to a per-model-instance resource since it's the same for every model instance
		uint objectId;
		float lodFactor;

		/// setup new instace
		virtual void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;

		/// update prior to drawing
		virtual void Update() override;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;
	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }

protected:
	friend class StreamModelPool;
	friend class ModelContext;

	/// load transform
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;

	Math::vec3 position;
	Math::quat rotate;
	Math::vec3 scale;
	Math::vec3 rotatePivot;
	Math::vec3 scalePivot;
	bool isInViewSpace;
	float minDistance;
	float maxDistance;
	bool useLodDistances;
	bool lockedToViewer;
};

ModelNodeInstanceCreator(TransformNode)

//------------------------------------------------------------------------------
/**
*/
inline void
TransformNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	ModelNode::Instance::Setup(node, parent);
	const TransformNode* tnode = static_cast<const TransformNode*>(node);
	this->transform.setposition(tnode->position);
	this->transform.setrotate(tnode->rotate);
	this->transform.setscale(tnode->scale);
	this->transform.setrotatepivot(tnode->rotatePivot);
	this->transform.setscalepivot(tnode->scalePivot);
}

} // namespace Models