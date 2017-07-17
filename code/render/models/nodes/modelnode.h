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
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "io/binaryreader.h"
#include "math/bbox.h"
namespace Models
{
class ModelLoader;
class Model;
class ModelNode : public Core::RefCounted
{
	__DeclareClass(ModelNode);
public:
	/// constructor
	ModelNode();
	/// destructor
	virtual ~ModelNode();

	/// return constant reference to children
	const Util::Array<Ptr<ModelNode>>& GetChildren() const;

protected:
	friend class ModelLoader;

	/// load data
	virtual bool Load(const Util::FourCC& tag, const Ptr<Models::ModelLoader>& loader, const Ptr<IO::BinaryReader>& reader);

	Util::StringAtom name;
	Ptr<ModelNode> parent;
	Util::Array<Ptr<ModelNode>> children;
	Math::bbox boundingBox;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<Models::ModelNode>>&
ModelNode::GetChildren() const
{
	return this->children;
}

} // namespace Models