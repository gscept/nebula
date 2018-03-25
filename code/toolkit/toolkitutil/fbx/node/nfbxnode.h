#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxNode
    
    Encapsulates an FBX node as a Nebula-friendly object
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/quaternion.h"
#include "math/float4.h"
#include "fbx/helpers/animsplitterhelper.h"
#include <fbxsdk.h>
#include "animutil/animbuilderclip.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "animutil/animbuilder.h"
#include "modelutil/modelattributes.h"

#define KEYS_PER_MS 40

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class NFbxScene;
class NFbxMeshNode;
class NFbxNode : public Core::RefCounted
{
	__DeclareClass(NFbxNode);
public:

	enum NodeType
	{
		Mesh,			// actual mesh node
		Joint,			// joint transform node
		Transform,		// plain transform node
		Light,			// light node

		Locator,		// locator node (unused)
		Effector,		// effector node (unused)

		UnknownType,	// unknown type, unasserted

		NumNodeTypes
	};

	/// constructor
	NFbxNode();
	/// destructor
	virtual ~NFbxNode();

	/// sets up node
	virtual void Setup(FbxNode* node, const Ptr<NFbxScene>& scene);
	/// discards the node
	virtual void Discard();

	/// extracts rotation, translation and scaling
	virtual void ExtractTransform();
	/// extracts rotation, translation and scaling
	virtual void ExtractTransform(const FbxMatrix& transform);

	/// sets the initial rotation (overrides extracted data)
	void SetInitialRotation(const Math::quaternion& rot);
	/// returns the initial rotation
	const Math::quaternion& GetInitialRotation() const;
	/// sets the initial position (overrides extracted data)
	void SetInitialPosition(const Math::float4& pos);
	/// returns the initial position
	const Math::float4& GetInitialPosition() const;
	/// sets the initial scale (overrides extracted data)
	void SetInitialScale(const Math::float4& scale);
	/// returns the initial scale
	const Math::float4& GetInitialScale() const;
	/// returns transform
	const Math::matrix44& GetTransform() const;

	/// adds a child to this node
	void AddChild(const Ptr<NFbxNode>& child);
	/// removes a child to this node
	void RemoveChild(const Ptr<NFbxNode>& child);
	/// returns a child to this node
	const Ptr<NFbxNode>& GetChild(IndexT index) const;
	/// returns the number of children
	const IndexT GetChildCount() const;
	/// return index of child
	const IndexT IndexOfChild(const Ptr<NFbxNode>& child);
	/// set the parent
	void SetParent(const Ptr<NFbxNode>& parent);
	/// returns pointer to parent
	const Ptr<NFbxNode>& GetParent() const;

	/// returns true if node is a part of the physics hierarchy
	const bool IsPhysics() const;

	/// returns the node type
	const NodeType GetNodeType() const;

	/// returns FBX node pointer
	FbxNode* GetNode() const;

	/// sets the node name
	void SetName(const Util::String& name);
	/// gets the node name
	const Util::String& GetName() const;

	/// generates animation clip
	virtual void GenerateAnimationClips(const Ptr<ToolkitUtil::ModelAttributes>& attributes);
	/// returns animation resource
	const ToolkitUtil::AnimBuilder& GetAnimation() const;

protected:

	/// extract animation curves from node
	virtual void ExtractAnimationCurves(FbxAnimStack* stack, Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int& postInfType, int& preInfType, int span);
	/// finds the total keyspan
	virtual void ExtractKeySpan(FbxAnimStack* stack, int& span);
	/// splits animation curves, returns true if splitter has any rules for this take
	bool SplitCurves(const Util::String& animStackName, const Ptr<ToolkitUtil::ModelAttributes>& attributes, ToolkitUtil::AnimBuilder& anim, const Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int span);

	/// recursively traverses nodes and calls DoMerge
	void MergeChildren(Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes);
	/// unparents children
	virtual void DoMerge(Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes);

	friend class NFbxScene;

	FbxScene*						fbxScene;
	FbxNode*						fbxNode;
	Util::Array<Ptr<NFbxNode> >		children;
	Ptr<NFbxNode>					parent;
	Ptr<NFbxScene>					scene;

	Util::String					name;

	Math::quaternion				rotation;
	Math::float4					position;
	Math::float4					scale;
	Math::matrix44					transform;

	ToolkitUtil::AnimBuilder		anim;
	NodeType						type;

	bool							isPhysics;
	bool							isAnimated;
	bool							isRoot;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxNode::SetInitialRotation( const Math::quaternion& rot )
{
	this->rotation = rot;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::quaternion& 
NFbxNode::GetInitialRotation() const
{
	return this->rotation;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxNode::SetInitialPosition( const Math::float4& pos )
{
	this->position = pos;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4& 
NFbxNode::GetInitialPosition() const
{
	return this->position;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxNode::SetInitialScale( const Math::float4& scale )
{
	this->scale = scale;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4& 
NFbxNode::GetInitialScale() const
{
	return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44& 
NFbxNode::GetTransform() const
{
	return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
inline FbxNode* 
NFbxNode::GetNode() const
{
	n_assert(this->fbxNode);
	return this->fbxNode;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<NFbxNode>&
NFbxNode::GetParent() const
{
	return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxNode::SetParent( const Ptr<NFbxNode>& parent )
{
	this->parent = parent;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT 
NFbxNode::IndexOfChild( const Ptr<NFbxNode>& child )
{
	IndexT index = this->children.FindIndex(child);
	n_assert(index != InvalidIndex);
	return index;
}

//------------------------------------------------------------------------------
/**
*/
inline const NFbxNode::NodeType 
NFbxNode::GetNodeType() const
{
	return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline const ToolkitUtil::AnimBuilder& 
NFbxNode::GetAnimation() const
{
	return this->anim;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
NFbxNode::IsPhysics() const
{
	return this->isPhysics;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxNode::SetName( const Util::String& name )
{
	n_assert(name.IsValid());
	this->name = name;
}


//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
NFbxNode::GetName() const
{
	return this->name;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------