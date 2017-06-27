#pragma once
//------------------------------------------------------------------------------
/**
	A model resource consists of nodes, each of which inhibit some information
	read from an .n3 file. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "resources/resource.h"
namespace Models
{
class ModelNode;
class Model : public Resources::Resource
{
	__DeclareClass(Model);
public:
	/// constructor
	Model();
	/// destructor
	virtual ~Model();

	/// finds node on name
	Ptr<ModelNode> FindNode(const Util::StringAtom& name);
private:

	Util::Dictionary<Util::StringAtom, Ptr<ModelNode>> nodes;
	Ptr<ModelNode> root;
};

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Models::ModelNode>
Model::FindNode(const Util::StringAtom& name)
{
	return this->nodes[name];
}

} // namespace Models