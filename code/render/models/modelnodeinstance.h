#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::ModelNodeInstance
    
    A ModelNodeInstance holds the per-instance data of a ModelNode and
    does most of the actually interesting Model rendering stuff.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "coregraphics/shaderstate.h"
#include "timing/time.h"
#include "math/bbox.h"
#include "debug/debugtimer.h"
#include "graphics/batchgroup.h"

//------------------------------------------------------------------------------
namespace Models
{
class ModelInstance;
class ModelNode;

class ModelNodeInstance : public Core::RefCounted
{
    __DeclareClass(ModelNodeInstance);
public:
    /// constructor
    ModelNodeInstance();
    /// destructor
    virtual ~ModelNodeInstance();

    /// setup the model node instance
    virtual void Setup(const Ptr<ModelInstance>& inst, const Ptr<ModelNode>& node, const Ptr<ModelNodeInstance>& parentNodeInst);
    /// discard the model node instance
    virtual void Discard();
    /// discard the model node instance and all of its children
    virtual void DiscardHierarchy();
    /// return true if the model node instance is valid
    bool IsValid() const;

    /// called from ModelEntity::OnNotifyCullingVisible
    virtual void OnNotifyCullingVisible(IndexT frameIndex, Timing::Time time);
    /// called from ModelEntity::OnRenderBefore
    virtual void OnRenderBefore(IndexT frameIndex, Timing::Time time);
    /// called during visibility resolve
	virtual void OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distanceToViewer);

    /// apply per-instance state prior to rendering
	virtual void ApplyState(IndexT frameIndex, const IndexT& pass);

    /// perform rendering
    virtual void Render();
    /// perform instanced rendering
    virtual void RenderInstanced(SizeT numInstances);

    /// get model node name
    const Util::StringAtom& GetName() const;
    /// return true if node has a parent
    bool HasParent() const;
    /// get parent node
    const Ptr<ModelNodeInstance>& GetParent() const;
    /// get child nodes
    const Util::Array<Ptr<ModelNodeInstance>>& GetChildren() const;
    /// return true if a direct child exists by name
    bool HasChild(const Util::StringAtom& name) const;
    /// get pointer to direct child by name
    const Ptr<ModelNodeInstance>& LookupChild(const Util::StringAtom& name) const;
    /// get modelnodeinstance by hierarchy path
    Ptr<ModelNodeInstance> LookupPath(const Util::String& path);
    /// get the ModelInstance we are attached to
    const Ptr<ModelInstance>& GetModelInstance() const;
    /// get the ModelNode we're associated with
    const Ptr<ModelNode>& GetModelNode() const;
    
    /// set the node instance's visibility
    void SetVisible(bool b, Timing::Time time, bool recursive = true);
    /// return true if node instance is set to visible
    bool IsVisible() const;
    /// get model node instance index for current frame
    IndexT GetModelNodeInstanceIndex() const;

#if NEBULA3_ENABLE_PROFILING
    /// start debug timer
    void StartDebugTimer();
    /// start debug timer
    void StopDebugTimer();
    /// start debug timer
    void ResetDebugTimer();
#endif

protected:
    friend class ModelInstance;

    /// set parent node
    void SetParent(const Ptr<ModelNodeInstance>& p);
    /// add a child node
    void AddChild(const Ptr<ModelNodeInstance>& c);
    /// render node specific debug shape
    virtual void RenderDebug();    
    /// called when the node becomes visible with current time
    virtual void OnShow(Timing::Time time);
    /// called when the node becomes invisible
    virtual void OnHide(Timing::Time time);

    Util::StringAtom name;
    Ptr<ModelInstance> modelInstance;
    Ptr<ModelNode> modelNode;
    Ptr<ModelNodeInstance> parent;
    Util::Array<Ptr<ModelNodeInstance> > children;
    Util::Dictionary<Util::StringAtom, IndexT> childIndexMap;
    bool isVisible;
    IndexT frameInstanceIndex; // index of this object for current render frame, is determined on visibility resolve

    _declare_timer(modelNodeInstanceTimer)
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ModelNodeInstance::IsValid() const
{
    return this->modelInstance.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
ModelNodeInstance::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelNodeInstance::SetParent(const Ptr<ModelNodeInstance>& p)
{
    this->parent = p;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ModelNodeInstance::HasParent() const
{
    return this->parent.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ModelNodeInstance>&
ModelNodeInstance::GetParent() const
{
    return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelNodeInstance> >&
ModelNodeInstance::GetChildren() const
{
    return this->children;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT 
ModelNodeInstance::GetModelNodeInstanceIndex() const
{
    return this->frameInstanceIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ModelNodeInstance::IsVisible() const
{
    return this->isVisible;
}

#if NEBULA3_ENABLE_PROFILING
//------------------------------------------------------------------------------
/**
*/
inline void 
ModelNodeInstance::StartDebugTimer()
{
    _start_accum_timer(modelNodeInstanceTimer);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ModelNodeInstance::StopDebugTimer()
{
    _stop_accum_timer(modelNodeInstanceTimer);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ModelNodeInstance::ResetDebugTimer()
{
    _reset_accum_timer(modelNodeInstanceTimer);
}
#endif
} // namespace Models
//------------------------------------------------------------------------------

