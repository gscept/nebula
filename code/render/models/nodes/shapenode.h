#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::MaterialShapeNode
  
    An alternative to ShapeNode when using materials instead of single shaders
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/    
#include "models/nodes/statenode.h"
#include "resources/managedmesh.h"
#include "resources/resourceloader.h"

//------------------------------------------------------------------------------
namespace Models
{
class ShapeNode : public StateNode
{
    __DeclareClass(ShapeNode);
public:
    /// constructor
    ShapeNode();
    /// destructor
    virtual ~ShapeNode();

    /// create a model node instance
    virtual Ptr<ModelNodeInstance> CreateNodeInstance() const;
    /// parse data tag (called by loader code)
    virtual bool ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader);
    /// called when resources should be loaded
    virtual void LoadResources(bool sync);
    /// called when resources should be unloaded
    virtual void UnloadResources();
    /// get overall state of contained resources (Initial, Loaded, Pending, Failed, Cancelled)
    virtual Resources::Resource::State GetResourceState() const;
    /// apply state shared by all my ModelNodeInstances
    virtual void ApplySharedState(IndexT frameIndex);
	
    /// set mesh resource id
    void SetMeshResourceId(const Resources::ResourceId& resId);
    /// get mesh resource id
    const Resources::ResourceId& GetMeshResourceId() const;
    /// set primitive group index
    void SetPrimitiveGroupIndex(IndexT i);
    /// get primitive group index
    IndexT GetPrimitiveGroupIndex() const;

    /// set optional resourceloader
    void SetMeshResourceLoader(const Ptr<Resources::ResourceLoader>& loader);

    /// get managed mesh
    const Ptr<Resources::ManagedMesh>& GetManagedMesh() const;

protected:
    Resources::ResourceId meshResId;
    IndexT primGroupIndex;
    Ptr<Resources::ManagedMesh> managedMesh;
    Ptr<Resources::ResourceLoader> resourceLoader;
};



//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Resources::ManagedMesh>& 
ShapeNode::GetManagedMesh() const
{
    return this->managedMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShapeNode::SetMeshResourceId(const Resources::ResourceId& resId)
{
    this->meshResId = resId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId&
ShapeNode::GetMeshResourceId() const
{
    return this->meshResId;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ShapeNode::SetPrimitiveGroupIndex(IndexT i)
{
    this->primGroupIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
ShapeNode::GetPrimitiveGroupIndex() const
{
    return this->primGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ShapeNode::SetMeshResourceLoader(const Ptr<Resources::ResourceLoader>& loader)
{
    this->resourceLoader = loader;
}

} // namespace Models
//------------------------------------------------------------------------------
