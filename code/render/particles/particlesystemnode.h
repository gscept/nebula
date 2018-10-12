#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemMaterialNode
  
    The ParticleSystemNode wraps a ParticleSystem object into a 
    ModelNode for rendering using materials.
    
    (C) 2011-2018 Individual contributors, see AUTHORS file
*/    
#include "models/nodes/statenode.h"
#include "resources/managedmesh.h"
#include "particles/particlesystem.h"
#include "particles/emitterattrs.h"

//------------------------------------------------------------------------------
namespace Particles
{
class ParticleSystemNode : public Models::StateNode
{
    __DeclareClass(ParticleSystemNode);
public:
    /// constructor
    ParticleSystemNode();
    /// destructor
    virtual ~ParticleSystemNode();

    /// create a model node instance
    virtual Ptr<Models::ModelNodeInstance> CreateNodeInstance() const;
    /// called when resources should be loaded
    virtual void LoadResources(bool sync);
    /// called when resources should be unloaded
    virtual void UnloadResources();
    /// get overall state of contained resources (Initial, Loaded, Pending, Failed, Cancelled)
    virtual Resources::Resource::State GetResourceState() const;
    /// called once when all pending resource have been loaded
    virtual void OnResourcesLoaded();
    /// parse data tag (called by loader code)
    virtual bool ParseDataTag(const Util::FourCC& fourCC, const Ptr<IO::BinaryReader>& reader);
	/// apply state shared by all my ModelNodeInstances
	virtual void ApplySharedState(IndexT frameIndex);

	/// change a mesh during runtime
	virtual void UpdateMeshResource(const Resources::ResourceId& resId);
    /// set resource id of emitter mesh
    void SetEmitterMeshResourceId(const Resources::ResourceId& resId);
    /// get emitter mesh resource id
    const Resources::ResourceId& GetEmitterMeshResourceId() const;
    /// set the primitive group index in the emitter mesh
    void SetPrimitiveGroupIndex(IndexT i);
    /// get the primitive group index in the emitter mesh
    IndexT GetPrimitiveGroupIndex() const;
    /// set emitter attributes
    void SetEmitterAttrs(const EmitterAttrs& attrs);
    /// get emitter attributes
    const EmitterAttrs& GetEmitterAttrs() const;
	/// get emitter mesh
	const Ptr<Resources::ManagedMesh> & GetEmitterMesh() const;

	void AddTexture(const Util::StringAtom & name, const Util::String & val);

private:
    /// helper function to parse an EnvelopeCurve from a data stream
    EnvelopeCurve ParseEnvelopeCurveData(const Ptr<IO::BinaryReader>& reader) const;

protected:    
    EmitterAttrs emitterAttrs;
    Resources::ResourceId meshResId;
    IndexT primGroupIndex;
    Ptr<Resources::ManagedMesh> managedMesh;
};

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemNode::SetEmitterMeshResourceId(const Resources::ResourceId& resId)
{
    this->meshResId = resId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId&
ParticleSystemNode::GetEmitterMeshResourceId() const
{
    return this->meshResId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Resources::ManagedMesh> &
ParticleSystemNode::GetEmitterMesh() const
{
	return this->managedMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemNode::SetPrimitiveGroupIndex(IndexT index)
{
    this->primGroupIndex = index;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
ParticleSystemNode::GetPrimitiveGroupIndex() const
{
    return this->primGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemNode::SetEmitterAttrs(const EmitterAttrs& attrs)
{
    this->emitterAttrs = attrs;
}

//------------------------------------------------------------------------------
/**
*/
inline const EmitterAttrs&
ParticleSystemNode::GetEmitterAttrs() const
{
    return this->emitterAttrs;
}

} // namespace Particles
//------------------------------------------------------------------------------
