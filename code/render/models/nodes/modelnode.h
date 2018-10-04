#pragma once
//------------------------------------------------------------------------------
/**
	Base class for all model nodes, which is the resource level instantiation
	of hierarchical model. Each node represents some level in the hierarchy,
	and the different type of node contain their specific data.

	The nodes have names for lookup, the name is setup during load. 

	A node is a part of an N-tree, meaning there are N children [0-infinity]
	for each node. The model itself keeps track of the root node, and a dictionary
	of all nodes and their corresponding names.
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "io/binaryreader.h"
#include "math/bbox.h"
#include "ids/id.h"
#include "memory/chunkallocator.h"
#include "models/model.h"

#define ModelNodeInstanceCreator(type) \
inline ModelNode::Instance* type::CreateInstance(byte* memory, const ModelNode::Instance* parent)\
{\
	auto node = new (memory) Instance();\
	node->Setup(this, parent);\
	return node;\
}

namespace Models
{

class StreamModelPool;
class ModelNode
{
public:
	struct Instance
	{
		const ModelNode::Instance* parent;	// pointer to parent
		ModelNode* node;				// pointer to resource-level node

		virtual void ApplyNodeInstanceState();
		virtual void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent);
	};

	/// constructor
	ModelNode();
	/// destructor
	virtual ~ModelNode();

	/// return constant reference to children
	const Util::Array<ModelNode*>& GetChildren() const;
	/// create an instance of a node, override in the leaf classes
	virtual ModelNode::Instance* CreateInstance(byte* memory, const Models::ModelNode::Instance* parent);

	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }

	/// apply node-level state
	virtual void ApplyNodeState();

protected:
	friend class StreamModelPool;
	friend class ModelContext;
	friend class ModelServer;

	/// load data
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);
	/// unload data
	virtual void Unload();
	/// call when model node data is finished loading (not accounting for secondary resources)
	virtual void OnFinishedLoading();

	/// discard node
	virtual void Discard();

	Util::StringAtom name;
	NodeType type;

	Models::ModelNode* parent;
	ModelId model;
	Util::Array<Models::ModelNode*> children;
	Math::bbox boundingBox;
	Util::StringAtom tag;

};


//------------------------------------------------------------------------------
/**
*/
inline ModelNode::Instance*
ModelNode::CreateInstance(byte* memory, const Models::ModelNode::Instance* parent)
{
	return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<ModelNode*>&
ModelNode::GetChildren() const
{
	return this->children;
}

} // namespace Models