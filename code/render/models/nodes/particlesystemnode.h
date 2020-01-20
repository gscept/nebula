#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemMaterialNode
  
    The ParticleSystemNode wraps a ParticleSystem object into a 
    ModelNode for rendering using materials.
    
    (C) 2011-2020 Individual contributors, see AUTHORS file
*/    
#include "shaderstatenode.h"
#include "particles/emitterattrs.h"
#include "particles/envelopesamplebuffer.h"
#include "particles/emittermesh.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/resourcetable.h"

//------------------------------------------------------------------------------
namespace Models
{
class ParticleSystemNode : public ShaderStateNode
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
	const Particles::EmitterMesh& GetEmitterMesh() const;
	/// get emitter sample buffer
	const Particles::EnvelopeSampleBuffer& GetSampleBuffer() const;

	struct Instance : public ShaderStateNode::Instance
	{
		enum DynamicOffsetType
		{
			ObjectTransforms,
			InstancingTransforms,
			Skinning,
			Particle
		};

		uint particleVboOffset;
		CoreGraphics::VertexBufferId particleVbo;
		CoreGraphics::PrimitiveGroup group;
		uint numParticles;

		IndexT particleConstantsIndex;

		Math::bbox boundingBox;

		/// update prior to drawing
		void Update() override;
		/// setup instance
		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;

		/// another draw function
		void Draw(const SizeT numInstances, Models::ModelNode::DrawPacket* packet);
	};

	/// create instance
	virtual ModelNode::Instance* CreateInstance(byte** memory, const Models::ModelNode::Instance* parent) override;
	/// get size of instance
	virtual const SizeT GetInstanceSize() const { return sizeof(Instance); }
private:
    /// helper function to parse an EnvelopeCurve from a data stream
    Particles::EnvelopeCurve ParseEnvelopeCurveData(const Ptr<IO::BinaryReader>& reader) const;

protected:    
	/// called once when all pending resource have been loaded
	virtual void OnFinishedLoading();
	/// parse data tag (called by loader code)
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;

	/// apply state
	void ApplyNodeState() override;
	
	Particles::EnvelopeSampleBuffer sampleBuffer;
	Particles::EmitterAttrs emitterAttrs;
	Particles::EmitterMesh emitterMesh;
    Resources::ResourceName meshResId;

	IndexT particleConstantsIndex;

	Util::StringAtom tag;
    IndexT primGroupIndex;
	CoreGraphics::MeshId mesh;
};

//------------------------------------------------------------------------------
/**
*/
inline const Particles::EmitterMesh&
ParticleSystemNode::GetEmitterMesh() const
{
	return this->emitterMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline const Particles::EnvelopeSampleBuffer& 
ParticleSystemNode::GetSampleBuffer() const
{
	return this->sampleBuffer;
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
ParticleSystemNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	ShaderStateNode::Instance::Setup(node, parent);
	ParticleSystemNode* pparent = static_cast<ParticleSystemNode*>(node);

	this->particleConstantsIndex = pparent->particleConstantsIndex;
	this->offsets.Resize(4);
	this->offsets[this->particleConstantsIndex] = 0;
	this->offsets[this->objectTransformsIndex] = 0;
	this->offsets[this->skinningTransformsIndex] = 0;
	this->offsets[this->instancingTransformsIndex] = 0;
	this->resourceTable = pparent->resourceTable;	
	this->particleVboOffset = 0;
	this->particleVbo = CoreGraphics::VertexBufferId::Invalid();
}

} // namespace Particles
//------------------------------------------------------------------------------
