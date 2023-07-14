#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::ParticleSystemMaterialNode
  
    The ParticleSystemNode wraps a ParticleSystem object into a 
    ModelNode for rendering using materials.

    @copyright
    (C) 2011-2020 Individual contributors, see AUTHORS file
*/    
#include "shaderstatenode.h"
#include "particles/emitterattrs.h"
#include "particles/envelopesamplebuffer.h"
#include "particles/emittermesh.h"
#include "coregraphics/resourcetable.h"

//------------------------------------------------------------------------------
namespace Models
{
class ModelContext;
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

    /// Create resource table
    static Util::FixedArray<CoreGraphics::ResourceTableId> CreateResourceTables();

private:
    /// helper function to parse an EnvelopeCurve from a data stream
    Particles::EnvelopeCurve ParseEnvelopeCurveData(const Ptr<IO::BinaryReader>& reader) const;

protected:    
    friend class Particles::ParticleContext;
    friend class Models::ModelContext;

    /// called once when all pending resource have been loaded
    virtual void OnFinishedLoading();
    /// parse data tag (called by loader code)
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;

    Particles::EnvelopeSampleBuffer sampleBuffer;
    Particles::EmitterAttrs emitterAttrs;
    Particles::EmitterMesh emitterMesh;
    Resources::ResourceName meshResId;

    Util::StringAtom tag;
    IndexT primGroupIndex;
    Resources::ResourceId meshResource;
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

} // namespace Particles
//------------------------------------------------------------------------------
