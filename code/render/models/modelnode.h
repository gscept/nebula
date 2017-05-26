#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::ModelNode
  
    Represents a transformation hierarchy element inside a Model. Subclasses
    of ModelNodes represent transformations and geometry of a 3D model
    arranged in 3d hierarchy (but not in a logical hierarchy of C++ object,
    instead model nodes live in a flat array to prevent recursive iteration).
    
    A ModelNode is roughly comparable to a nSceneNode in Nebula2.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "resources/resource.h"
#include "graphics/batchgroup.h"
#include "coregraphics/shaderstate.h"
#include "models/visresolvecontainer.h"
#include "math/bbox.h"
#include "io/binaryreader.h"
#include "materials/surfacename.h"

#if NEBULA3_ENABLE_PROFILING
#include "debug/debugtimer.h"
#include "debug/debugcounter.h"
#endif

//------------------------------------------------------------------------------
namespace Models
{
class Model;
class ModelInstance;
class ModelNodeInstance;

class ModelNode : public Core::RefCounted
{
    __DeclareClass(ModelNode);
public:
    /// constructor
    ModelNode();
    /// destructor
    virtual ~ModelNode();

    /// create a model node instance
    virtual Ptr<ModelNodeInstance> CreateNodeInstance() const;
    /// recursively create model node instance and child model node instances
    Ptr<ModelNodeInstance> CreateNodeInstanceHierarchy(const Ptr<ModelInstance>& modelInst);
    /// called when attached to model node
    virtual void OnAttachToModel(const Ptr<Model>& model);
    /// called when removed from model node
    virtual void OnRemoveFromModel();
    /// called when resources should be loaded
    virtual void LoadResources(bool sync);
    /// called when resources should be unloaded
    virtual void UnloadResources();
	/// called when resources should be reloaded
	virtual void ReloadResources();
    /// called once when all pending resource have been loaded
    virtual void OnResourcesLoaded();
    /// begin parsing data tags
    virtual void BeginParseDataTags();
    /// parse data tag (called by loader code)
    virtual bool ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader);
    /// finish parsing data tags
    virtual void EndParseDataTags();
    /// apply state shared by all my ModelNodeInstances
    virtual void ApplySharedState(IndexT frameIndex);
    /// get overall state of contained resources (Initial, Loaded, Pending, Failed, Cancelled)
    virtual Resources::Resource::State GetResourceState() const;

    /// return true if currently attached to a Model
    bool IsAttachedToModel() const;
    /// get model this node is attached to
    const Ptr<Model>& GetModel() const;

    /// sets the resourceStreamingLevelOfDetail but only if the given value is bigger than the current one (reseted on frame-start)
    void SetResourceStreamingLevelOfDetail(float factor);
    /// resets all screen space stats e.g. size
    void ResetScreenSpaceStats();
    /// set bounding box
    void SetBoundingBox(const Math::bbox& b);
    /// get bounding box of model node
    const Math::bbox& GetBoundingBox() const;
    /// set model node name
    void SetName(const Util::StringAtom& n);
    /// get model node name
    const Util::StringAtom& GetName() const;

    /// set parent node
    void SetParent(const Ptr<ModelNode>& p);
    /// get parent node
    const Ptr<ModelNode>& GetParent() const;
    /// return true if node has a parent
    bool HasParent() const;
    /// add a child node
    void AddChild(const Ptr<ModelNode>& c);
    /// get child nodes
    const Util::Array<Ptr<ModelNode> >& GetChildren() const;
    /// return true if a direct child exists by name
    bool HasChild(const Util::StringAtom& name) const;
    /// get pointer to direct child by name
    const Ptr<ModelNode>& LookupChild(const Util::StringAtom& name) const;
	/// Remove child node, use with care! need to invalidate instance after
	void RemoveChild(const Ptr<ModelNode> & c);
	/// fins child recursively
	const Ptr<ModelNode> FindChild(const Util::StringAtom& name) const;

    /// called by model node instance on NotifyVisible()
    void AddVisibleNodeInstance(IndexT frameIndex, const Materials::SurfaceName::Code& surfaceName, const Ptr<ModelNodeInstance>& nodeInst);
    /// get visible model node instances
    const Util::Array<Ptr<ModelNodeInstance>>& GetVisibleModelNodeInstances(const Materials::SurfaceName::Code& surfaceName) const;

    /// has string attr 
    bool HasStringAttr(const Util::StringAtom& attrId) const;
    /// get string attr    
    const Util::StringAtom& GetStringAttr(const Util::StringAtom& attrId) const;
    /// add string attribute
    void SetStringAttr(const Util::StringAtom& attrId, const Util::StringAtom& value);

#if NEBULA3_ENABLE_PROFILING
	/// start profiling timer
	void StartTimer();
	/// stop profiling timer
	void StopTimer();
	/// increase render count
	void IncrementDraws();
#endif

protected:
    /// recursively create node instance hierarchy
    virtual Ptr<ModelNodeInstance> RecurseCreateNodeInstanceHierarchy(const Ptr<ModelInstance>& modelInst, const Ptr<ModelNodeInstance>& parentNodeInst);

    Ptr<Model> model;
    Util::StringAtom name;
    Ptr<ModelNode> parent;
    Util::Array<Ptr<ModelNode>> children;
    Util::Dictionary<Util::StringAtom, IndexT> childIndexMap;
    VisResolveContainer<ModelNodeInstance, Materials::SurfaceName::Code, Materials::SurfaceName::MaxNumSurfaceNames> visibleModelNodeInstances;
    Util::Dictionary<Util::StringAtom, Util::StringAtom> stringAttrs;
    Math::bbox boundingBox;
    bool inLoadResources;

    /// factor between 0.0 (close) and 1.0 (far away) describing the distance to camera (used for decision of max needed mipMap)
    float resourceStreamingLevelOfDetail;

#if NEBULA3_ENABLE_PROFILING
	_declare_timer(debugTimer);
	_declare_counter(debugCounter);
#endif
};



//------------------------------------------------------------------------------
/**
*/
inline void
ModelNode::SetResourceStreamingLevelOfDetail(float factor)
{
    if (this->resourceStreamingLevelOfDetail < factor)
    {
        this->resourceStreamingLevelOfDetail = factor;
    }
}

//------------------------------------------------------------------------------
/**
    Reset resourceStreamingLevelOfDetail to -1.0 as we are able to recognize invisible items this way.
    (visible items will overwrite this value with a value >= 0.0)
*/
inline void
ModelNode::ResetScreenSpaceStats()
{
    this->resourceStreamingLevelOfDetail = -1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelNode::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
ModelNode::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelNode::SetParent(const Ptr<ModelNode>& p)
{
    this->parent = p;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ModelNode::HasParent() const
{
    return this->parent.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<ModelNode>&
ModelNode::GetParent() const
{
    return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelNode> >&
ModelNode::GetChildren() const
{
    return this->children;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ModelNodeInstance>>&
ModelNode::GetVisibleModelNodeInstances(const Materials::SurfaceName::Code& code) const
{
    return this->visibleModelNodeInstances.Get(code);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelNode::SetBoundingBox(const Math::bbox& b)
{
    this->boundingBox = b;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox&
ModelNode::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
ModelNode::HasStringAttr(const Util::StringAtom& attrId) const
{
    return this->stringAttrs.Contains(attrId);
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom& 
ModelNode::GetStringAttr(const Util::StringAtom& attrId) const
{
    return this->stringAttrs[attrId];
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelNode::SetStringAttr(const Util::StringAtom& attrId, const Util::StringAtom& value)
{
    if (this->HasStringAttr(attrId))
    {
        this->stringAttrs[attrId] = value;
    }
    else
    {
        this->stringAttrs.Add(attrId, value);
    }
}

} // namespace Models
//------------------------------------------------------------------------------

