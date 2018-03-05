#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemMaterialNode
  
    The ParticleSystemNode wraps a ParticleSystem object into a 
    ModelNode for rendering using materials.
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/    
#include "shaderstatenode.h"
#include "particles/particlesystem.h"
#include "particles/emitterattrs.h"

//------------------------------------------------------------------------------
namespace Models
{
class ParticleSystemNode : public Models::ShaderStateNode
{
public:
    /// constructor
    ParticleSystemNode();
    /// destructor
    virtual ~ParticleSystemNode();

    /// called once when all pending resource have been loaded
    virtual void OnFinishedLoading();
    /// parse data tag (called by loader code)
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);
	/// apply state shared by all my ModelNodeInstances
	virtual void ApplySharedState(IndexT frameIndex);

	/// change a mesh during runtime
	virtual void UpdateMeshResource(const Resources::ResourceName& msh);
    /// get emitter mesh resource id
    const CoreGraphics::MeshId& GetEmitterMeshResourceId() const;
    /// get the primitive group index in the emitter mesh
    IndexT GetPrimitiveGroupIndex() const;
    /// set emitter attributes
    void SetEmitterAttrs(const Particles::EmitterAttrs& attrs);
    /// get emitter attributes
    const Particles::EmitterAttrs& GetEmitterAttrs() const;
	/// get emitter mesh
	const CoreGraphics::MeshId GetEmitterMesh() const;

	struct Instance : public ShaderStateNode::Instance
	{
		Ids::Id32 particleSystemId;
		CoreGraphics::ShaderId particleShader;
		CoreGraphics::ShaderConstantId emitterOrientationVar;
		CoreGraphics::ShaderConstantId billBoardVar;
		CoreGraphics::ShaderConstantId bboxCenterVar;
		CoreGraphics::ShaderConstantId bboxSizeVar;
		CoreGraphics::ShaderConstantId animPhasesVar;
		CoreGraphics::ShaderConstantId animsPerSecVar;
		IndexT bufferIndex;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const;
private:
    /// helper function to parse an EnvelopeCurve from a data stream
    Particles::EnvelopeCurve ParseEnvelopeCurveData(const Ptr<IO::BinaryReader>& reader) const;

protected:    

	Particles::EmitterAttrs emitterAttrs;
    Resources::ResourceName meshResId;
	Util::StringAtom tag;
    IndexT primGroupIndex;
	CoreGraphics::MeshId mesh;
};

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::MeshId
ParticleSystemNode::GetEmitterMesh() const
{
	return this->mesh;
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
ParticleSystemNode::SetEmitterAttrs(const Particles::EmitterAttrs& attrs)
{
    this->emitterAttrs = attrs;
}

//------------------------------------------------------------------------------
/**
*/
inline const Particles::EmitterAttrs&
ParticleSystemNode::GetEmitterAttrs() const
{
    return this->emitterAttrs;
}

//------------------------------------------------------------------------------
/**
*/
inline ModelNode::Instance*
ParticleSystemNode::CreateInstance(Memory::ChunkAllocator<0xFFF>& alloc) const
{
	return alloc.Alloc<ParticleSystemNode::Instance>();
}

} // namespace Particles
//------------------------------------------------------------------------------
