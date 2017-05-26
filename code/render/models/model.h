#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::Model
    
    A Model represents the template for a renderable object, consisting
    of a hierarchy of ModelNodes which represent transformations and shapes.
    Models should generally be created through the ModelServer, which 
    guarantees that a given Model is only loaded once into memory. To
    render a Model, at least one ModelInstance must be created from the
    Model. Usually one ModelInstance is created per game object. Generally
    speaking, all per-instance data lives in the ModelInstance objects, while
    all constant data lives in the Model object. 

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "math/bbox.h"
#include "models/modelnode.h"
#include "models/visresolvecontainer.h"

//------------------------------------------------------------------------------

namespace Models
{
class ModelReader;
class ModelWriter;
class ModelInstance;


class Model : public Resources::Resource
{
    __DeclareClass(Model);
public:
    /// constructor
    Model();
    /// destructor
    virtual ~Model();

    /// unload the resource, or cancel the pending load
    virtual void Unload();
	/// reload the resource
	virtual void Reload();

    /// load node resources (meshes, textures, shaders, ...)
    void LoadResources();
    /// unload node resources
    void UnloadResources();
	/// reload node resources
	void ReloadResources();
    /// check if all resources have been loaded, return true if yes
    bool CheckPendingResources();

    /// update model's bounding box from model nodes
    void UpdateBoundingBox();
    /// set the model's local bounding box
    void SetBoundingBox(const Math::bbox& b);
    /// get the model's local bounding box
    const Math::bbox& GetBoundingBox() const;

    /// lookup a ModelNode in the Model by path, returns invalid pointer if not found
    Ptr<ModelNode> LookupNode(const Util::String& path) const;
    /// attach a model node to the Model
    void AttachNode(const Ptr<ModelNode>& node);
    /// remove a model node from the Model
    void RemoveNode(const Ptr<ModelNode>& node);
    /// access to model nodes
    const Util::Array<Ptr<ModelNode> >& GetNodes() const;
    /// get root node (always at index 0)
    const Ptr<ModelNode>& GetRootNode() const;

    /// create a ModelInstance of the Model
    Ptr<ModelInstance> CreateInstance();
    /// create a ModelInstance from a hierarchy sub-tree
    Ptr<ModelInstance> CreatePartialInstance(const Util::StringAtom& rootNodePath, const Math::matrix44& rootNodeOffsetMatrix);
    /// discard an instance
    void DiscardInstance(Ptr<ModelInstance> modelInstance);
    /// get all attached model instances
    const Util::Array<Ptr<ModelInstance> >& GetInstances() const;

private:
    friend class VisResolver;
    friend class ModelServer;
    friend class ModelInstance;
    friend class ModelNode;

    /// called once when all pending resources have been loaded
    void OnResourcesLoaded();
    /// get visible model nodes
    const Util::Array<Ptr<ModelNode>>& GetVisibleModelNodes(const Materials::SurfaceName::Code& code) const;
    /// lookup node index by path
    IndexT GetNodeIndexByName(const Util::String& path) const;
    /// called by ModelNode as a result of NotifyVisible()
    void AddVisibleModelNode(IndexT frameIndex, const Materials::SurfaceName::Code& code, const Ptr<ModelNode>& modelNode);

    Math::bbox boundingBox;
    Util::Array<Ptr<ModelNode> > nodes;
    Util::Array<Ptr<ModelInstance> > instances;
	VisResolveContainer<ModelNode, Materials::SurfaceName::Code, Materials::SurfaceName::MaxNumSurfaceNames> visibleModelNodes;
    bool inLoadResources;
    bool resourcesLoaded;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Model::SetBoundingBox(const Math::bbox& b)
{
    this->boundingBox = b;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox&
Model::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
    Get a pointer to the root node. This is always
    the first node.
*/
inline const Ptr<ModelNode>&
Model::GetRootNode() const
{
    n_assert(!this->nodes.IsEmpty());
    n_assert(!this->nodes[0]->HasParent());
    return this->nodes[0];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelNode> >&
Model::GetNodes() const
{
    return this->nodes;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelNode> >&
Model::GetVisibleModelNodes(const Materials::SurfaceName::Code& code) const
{
    return this->visibleModelNodes.Get(code);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelInstance> >&
Model::GetInstances() const
{
    return this->instances;
}

} // namespace Models
//------------------------------------------------------------------------------

    