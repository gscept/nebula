#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::ModelInstance
    
    A ModelInstance contains the per-instance data for rendering a Model. 
    Usually there is one ModelInstance created per game object.
    
    A ModelInstance is roughly comparable to a Nebula2 nRenderContext.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "timing/time.h"
#include "models/model.h"
#include "models/modelnodeinstance.h"

namespace Graphics
{
    class ModelEntity;
	class BillboardEntity;
	class MeshEntity;
}

namespace Physics
{
	class PhysicsNodeInstance;
}
//------------------------------------------------------------------------------
namespace Models
{
class ModelInstance : public Core::RefCounted
{
    __DeclareClass(ModelInstance);
public:
    /// constructor
    ModelInstance();
    /// destructor
    virtual ~ModelInstance();

    /// return true if currently valid
    bool IsValid() const;

	/// set instance bool
	void SetInstanced(bool b);
	/// return true if model instance is to be rendered instanced
	bool IsInstanced() const;
	/// set picking id
	void SetPickingId(IndexT i);
	/// get picking id
	const IndexT GetPickingId() const;

    /// get the Model this instance was created from
    const Ptr<Model>& GetModel() const;
    /// get the ModelEntity which owns this instance
    const Ptr<Graphics::ModelEntity>& GetModelEntity() const;

    /// set world space transform of the instance
    void SetTransform(const Math::matrix44& m);
    /// get world space transform
    const Math::matrix44& GetTransform() const;

    /// lookup a node instance, return invalid ptr if not exists, this method is SLOW
    Ptr<ModelNodeInstance> LookupNodeInstance(const Util::String& path) const;
    /// internal node lookup function
    Ptr<ModelNodeInstance> LookupNodeInstanceHelper(const Util::String& path, const Ptr<ModelNodeInstance>& node) const;
    /// get all node instances
    const Util::Array<Ptr<ModelNodeInstance> >& GetNodeInstances() const;
    /// get pointer to top-level node instance
    const Ptr<ModelNodeInstance>& GetRootNodeInstance() const;
    /// attach a node instance which has already been setup
    void AttachNodeInstance(const Ptr<ModelNodeInstance>& nodeInst);
    /// remove a node instance, does not discard the node instance!
    void RemoveNodeInstance(const Ptr<ModelNodeInstance>& nodeInst);

    /// called from ModelEntity::OnNotifyCullingVisible
    void OnNotifyCullingVisible(IndexT frameIndex, Timing::Time time);
    /// called from ModelEntity::OnRenderBefore
    void OnRenderBefore(IndexT frameIndex, Timing::Time time);
    /// called during visibility resolve
	void OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, const Math::matrix44& cameraTransform, bool updateLod);
    /// calls ModelInstance::OnVisibilityResolve(...) + updating screen space stats
    void UpdateRenderStats(const Math::matrix44& cameraTransform);

    /// render node specific debug shape
    void RenderDebug();

protected:
    friend class Model;
    friend class VisResolver;
    friend class Graphics::ModelEntity;
	friend class Graphics::BillboardEntity;
	friend class Graphics::MeshEntity;

    /// setup the ModelInstance from a root model node
    void Setup(const Ptr<Model>& model, const Ptr<ModelNode>& rootModelNode);
    /// cleanup and unlink from model
    void Discard();
    /// set pointer to ModelEntity which owns this instance
    void SetModelEntity(const Ptr<Graphics::ModelEntity>& mdlEntity);
    /// called when the entity becomes visible with current time
    void OnShow(Timing::Time time);
    /// called when the entity becomes invisible
    void OnHide(Timing::Time time);

    Ptr<Model> model;
    Ptr<ModelNode> modelNode;
    Ptr<Graphics::ModelEntity> modelEntity;
    Math::matrix44 transform;
    Util::Array<Ptr<ModelNodeInstance> > nodeInstances;	
	bool instanced;
	IndexT pickingId;
	IndexT visibilityResolveIndex;
	float viewDistanceThisFrame;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ModelInstance::IsValid() const
{
    return this->model.isvalid();
}


//------------------------------------------------------------------------------
/**
*/
inline void 
ModelInstance::SetInstanced( bool b )
{
	this->instanced = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
ModelInstance::IsInstanced() const
{
	return this->instanced;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ModelInstance::SetPickingId( IndexT i )
{
	this->pickingId = i;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT
ModelInstance::GetPickingId() const
{
	return this->pickingId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Model>&
ModelInstance::GetModel() const
{
    return this->model;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::matrix44&
ModelInstance::GetTransform() const
{
    return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelNodeInstance> >&
ModelInstance::GetNodeInstances() const
{
    return this->nodeInstances;
}

} // namespace Models
//------------------------------------------------------------------------------

