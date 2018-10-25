#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemMaterialNode
  
    The ParticleSystemNode wraps a ParticleSystem object into a 
    ModelNode for rendering using materials.
    
    (C) 2011-2018 Individual contributors, see AUTHORS file
*/    
#include "shaderstatenode.h"
#include "particles/particlesystem.h"
#include "particles/emitterattrs.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/resourcetable.h"

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
		
		CoreGraphics::ResourceTableId resourceTable;
		CoreGraphics::ConstantBufferId cbo;
		CoreGraphics::ConstantBinding emitterOrientationVar;
		CoreGraphics::ConstantBinding billboardVar;
		CoreGraphics::ConstantBinding bboxCenterVar;
		CoreGraphics::ConstantBinding bboxSizeVar;
		CoreGraphics::ConstantBinding animPhasesVar;
		CoreGraphics::ConstantBinding animsPerSecVar;
		
		uint32 instance;
		Util::FixedArray<uint32> offsets;

		void Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent) override;
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
	virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader);

	
	Particles::EmitterAttrs emitterAttrs;
    Resources::ResourceName meshResId;
	Util::StringAtom tag;
    IndexT primGroupIndex;
	CoreGraphics::MeshId mesh;

	CoreGraphics::ShaderId shader;
	CoreGraphics::ConstantBufferId cbo;
	IndexT cboIndex;
	CoreGraphics::ResourceTableId resourceTable;
	CoreGraphics::ConstantBinding emitterOrientationVar;
	CoreGraphics::ConstantBinding billboardVar;
	CoreGraphics::ConstantBinding bboxCenterVar;
	CoreGraphics::ConstantBinding bboxSizeVar;
	CoreGraphics::ConstantBinding animPhasesVar;
	CoreGraphics::ConstantBinding animsPerSecVar;
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
ParticleSystemNode::Instance::Setup(Models::ModelNode* node, const Models::ModelNode::Instance* parent)
{
	TransformNode::Instance::Setup(node, parent);
	ParticleSystemNode* pparent = static_cast<ParticleSystemNode*>(node);
	CoreGraphics::ConstantBufferId cbo = pparent->cbo;
	this->cbo = cbo;
	this->resourceTable = pparent->resourceTable;
	this->offsets.Resize(2);
	bool rebind = CoreGraphics::ConstantBufferAllocateInstance(cbo, this->offsets[0], this->instance);
	this->offsets[1] = 0; // instancing offset
	if (rebind)
	{
		CoreGraphics::ResourceTableSetConstantBuffer(pparent->resourceTable, { pparent->cbo, pparent->cboIndex, 0, true, false, -1, 0 });
		CoreGraphics::ResourceTableCommitChanges(pparent->resourceTable);
	}
	this->emitterOrientationVar = pparent->emitterOrientationVar;
	this->billboardVar = pparent->billboardVar;
	this->bboxCenterVar = pparent->bboxCenterVar;
	this->bboxSizeVar = pparent->bboxSizeVar;
	this->animPhasesVar = pparent->animPhasesVar;
	this->animsPerSecVar = pparent->animsPerSecVar;
}

} // namespace Particles
//------------------------------------------------------------------------------
