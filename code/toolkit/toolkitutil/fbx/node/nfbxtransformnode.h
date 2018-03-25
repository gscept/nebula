#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxTransformNode
    
    Wraps an FBX transform node as a Nebula-style node
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "fbx/node/nfbxnode.h"
#include "math/bbox.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class NFbxTransformNode : public NFbxNode
{
	__DeclareClass(NFbxTransformNode);
public:
	/// constructor
	NFbxTransformNode();
	/// destructor
	virtual ~NFbxTransformNode();

	/// sets up transformation node
	void Setup(FbxNode* node, const Ptr<NFbxScene>& scene);

	/// returns bounding box for transform group
	const Math::bbox& GetBoundingBox() const;	
	/// returns pivot for transform group
	const Math::point& GetRotationPivot() const;


protected:

	/// merges children
	void DoMerge(Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes);

	FbxNull*				fbxTransform;
	Math::point				pivot;
	Math::bbox				boundingBox;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox& 
NFbxTransformNode::GetBoundingBox() const
{
	return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::point& 
NFbxTransformNode::GetRotationPivot() const
{
	return this->pivot;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------