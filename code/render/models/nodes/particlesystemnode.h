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
class ParticleSystemNode : public TransformNode
{
public:
    /// constructor
    ParticleSystemNode();
    /// destructor
    virtual ~ParticleSystemNode();

	/// change a mesh during runtime
	virtual void UpdateMeshResource(const Resources::ResourceName& msh);
    /// get the primitive group index in the emitter mesh
    IndexT GetPrimitiveGroupIndex() const;
    /// set emitter attributes
    void SetEmitterAttrs(const Particles::EmitterAttrs& attrs);
    /// get emitter attributes
    const Particles::EmitterAttrs& GetEmitterAttrs() const;
	/// get emitter mesh
	const CoreGraphics::MeshId GetEmitterMesh() const;

	struct Instance : public TransformNode::Instance
	{
		Ids::Id32 particleSystemId;
		CoreGraphics::ShaderStateId particleShader;
		CoreGraphics::ShaderConstantId emitterOrientationVar;
		CoreGraphics::ShaderConstantId billboardVar;
		CoreGraphics::ShaderConstantId bboxCenterVar;
		CoreGraphics::ShaderConstantId bboxSizeVar;
		CoreGraphics::ShaderConstantId animPhasesVar;
		CoreGraphics::ShaderConstantId animsPerSecVar;
		IndexT bufferIndex;

		void Setup(const Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(Memory::ChunkAllocator<MODEL_INSTANCE_MEMORY_CHUNK_SIZE>& alloc, const Models::ModelNode::Instance* parent) const;
private:
    /// helper function to parse an EnvelopeCurve from a data stream
    Particles::EnvelopeCurve ParseEnvelopeCurveData(const Ptr<IO::BinaryReader>& reader) const;

protected:    
	/// called once when all pending resource have been loaded
	virtual void OnFinishedLoading();
	/// parse data tag (called by loader code)
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	CoreGraphics::ShaderId shader;
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

ModelNodeInstanceCreator(ParticleSystemNode)

//------------------------------------------------------------------------------
/**
*/
inline void
ParticleSystemNode::Instance::Setup(const Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	TransformNode::Instance::Setup(node, parent);
	this->particleShader = ShaderCreateState(static_cast<const ParticleSystemNode*>(node)->shader, { NEBULAT_DYNAMIC_OFFSET_GROUP }, false);
	this->emitterOrientationVar = CoreGraphics::ShaderStateGetConstant(this->particleShader, "EmitterTransform");
	this->billboardVar = CoreGraphics::ShaderStateGetConstant(this->particleShader, "Billboard");
	this->bboxCenterVar = CoreGraphics::ShaderStateGetConstant(this->particleShader, "BBoxCenter");
	this->bboxSizeVar = CoreGraphics::ShaderStateGetConstant(this->particleShader, "BBoxSize");
	this->animPhasesVar = CoreGraphics::ShaderStateGetConstant(this->particleShader, "NumAnimPhases");
	this->animsPerSecVar = CoreGraphics::ShaderStateGetConstant(this->particleShader, "AnimFramesPerSecond");
	this->type = ParticleSystemNodeType;
}

} // namespace Particles
//------------------------------------------------------------------------------
