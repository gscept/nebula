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
#include "math/bbox.h"
namespace Models
{
class ModelPool;
class ModelServer;
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
	/// get model bounding box
	const Math::bbox& GetBoundingBox() const;
private:
	/// the model loader is responsible for filling this class
	friend class ModelPool;
	friend class ModelContext;

	Math::bbox boundingBox;
	Util::Dictionary<Util::StringAtom, Ptr<ModelNode>> nodes;
	Ptr<ModelNode> root;
	Util::Array<Resources::ResourceId> resources;
};

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Models::ModelNode>
Model::FindNode(const Util::StringAtom& name)
{
	return this->nodes[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox&
Model::GetBoundingBox() const
{
	return this->boundingBox;
}

} // namespace Models