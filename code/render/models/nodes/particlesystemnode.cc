//------------------------------------------------------------------------------
//  particlesystemmaterialnode.cc
//  (C) 2011-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/particle.h"
#include "particlesystemnode.h"
#include "resources/resourceserver.h"
#include "models/model.h"
#include "coregraphics/mesh.h"
#include "coregraphics/shaderserver.h"
#include "particles/particlecontext.h"
#include "clustering/clustercontext.h"
#include "lighting/lightcontext.h"

#include "particle.h"

namespace Models
{
using namespace Particles;
using namespace Resources;
using namespace CoreGraphics;
using namespace Models;
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
ParticleSystemNode::ParticleSystemNode() :
    primGroupIndex(InvalidIndex),
    mesh(Resources::InvalidResourceId)
{
    this->type = ParticleSystemNodeType;
    this->bits = HasTransformBit | HasStateBit;
}

//------------------------------------------------------------------------------
/**
*/
ParticleSystemNode::~ParticleSystemNode()
{
    n_assert(this->mesh == Resources::InvalidResourceId);
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNode::UpdateMeshResource(const Resources::ResourceName& resName)
{
    // Discard old mesh, but be careful to not discard the default emitter
    if (this->mesh.cacheInstanceId != 0xFFFFFF && this->mesh.cacheIndex != 0xFF)
        Resources::DiscardResource(this->mesh);
    
    // load new mesh
    this->meshResId = resName;

    if (this->meshResId.IsValid())
        this->mesh = Resources::CreateResource(this->meshResId, this->tag, nullptr, nullptr, false);
    else
    {
        this->mesh.cacheIndex = -1;
        this->mesh.cacheInstanceId = -1;
        this->mesh.resourceId = ParticleContext::DefaultEmitterMesh.resourceId;
        this->mesh.resourceType = ParticleContext::DefaultEmitterMesh.resourceType;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleSystemNode::OnFinishedLoading()
{
    // skip state node intentionally because we don't want its setup
    TransformNode::OnFinishedLoading();

    // load surface ourselves since state node does the resource table setup too, but we need it explicit
    this->material = Resources::CreateResource(this->materialName, this->tag, nullptr, nullptr, true);

    float activityDist = this->emitterAttrs.GetFloat(EmitterAttrs::ActivityDistance) * 0.5f;

    // calculate bounding box using activity distance
    this->boundingBox.set(this->boundingBox.center(), Math::vector(activityDist, activityDist, activityDist));

    // setup sample buffer and emitter mesh
    this->sampleBuffer.Setup(this->emitterAttrs, ParticleSystemNumEnvelopeSamples);
    this->emitterMesh.Setup(this->mesh, this->primGroupIndex);

    ShaderId shader = ShaderServer::Instance()->GetShader("shd:particle.fxb"_atm);
    BufferId cbo = GetGraphicsConstantBuffer();
    this->objectTransformsIndex = ::Particle::Table_DynamicOffset::ObjectBlock::SLOT;
    this->instancingTransformsIndex = ::Particle::Table_DynamicOffset::InstancingBlock::SLOT;
    this->skinningTransformsIndex = ::Particle::Table_DynamicOffset::JointBlock::SLOT;
    this->particleConstantsIndex = ::Particle::Table_DynamicOffset::ParticleObjectBlock::SLOT;
    this->resourceTable = ShaderCreateResourceTable(shader, NEBULA_DYNAMIC_OFFSET_GROUP, 256);
    ResourceTableSetConstantBuffer(this->resourceTable, { cbo, this->particleConstantsIndex, 0, sizeof(::Particle::ParticleObjectBlock), 0, false, true });
    ResourceTableCommitChanges(this->resourceTable);
}

//------------------------------------------------------------------------------
/**
*/
std::function<void(const CoreGraphics::CmdBufferId)>
ParticleSystemNode::GetApplyFunction()
{
    return [](const CoreGraphics::CmdBufferId)
    {
        n_error("Invalid function '%s' called, it should be set in ParticleContext", __FUNCTION__);
        // This function is provided in ParticleContext
    };
}

//------------------------------------------------------------------------------
/**
*/
std::function<const CoreGraphics::PrimitiveGroup()>
ParticleSystemNode::GetPrimitiveGroupFunction()
{
    return []()
    {
        return ParticleContext::ParticleContext::GetParticlePrimitiveGroup();
    };
}

//------------------------------------------------------------------------------
/**
    Helper function for ParseDataTag, parses the data elements of 
    an EnvelopeCurve from the data stream and returns an EnvelopeCurve
    object.
*/
Particles::EnvelopeCurve
ParticleSystemNode::ParseEnvelopeCurveData(const Ptr<IO::BinaryReader>& reader) const
{
    float val0    = reader->ReadFloat();
    float val1    = reader->ReadFloat();
    float val2    = reader->ReadFloat();
    float val3    = reader->ReadFloat();
    float keyPos0 = reader->ReadFloat();
    float keyPos1 = reader->ReadFloat();
    float freq    = reader->ReadFloat();
    float amp     = reader->ReadFloat();
    EnvelopeCurve::ModFunc modFunc = (EnvelopeCurve::ModFunc) reader->ReadInt();
    EnvelopeCurve envCurve;
    envCurve.Setup(val0, val1, val2, val3, keyPos0, keyPos1, freq, amp, modFunc);
    return envCurve;
}

//------------------------------------------------------------------------------
/**
*/
bool
ParticleSystemNode::Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate)
{
    bool retval = true;
    if (FourCC('EFRQ') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::EmissionFrequency, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PLFT') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::LifeTime, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PSMN') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::SpreadMin, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PSMX') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::SpreadMax, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PSVL') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::StartVelocity, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PRVL') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::RotationVelocity, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PSZE') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::Size, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PMSS') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::Mass, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PTMN') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::TimeManipulator, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PVLF') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::VelocityFactor, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PAIR') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::AirResistance, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PRED') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::Red, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PGRN') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::Green, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PBLU') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::Blue, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PALP') == fourcc)
    {
        this->emitterAttrs.SetEnvelope(EmitterAttrs::Alpha, this->ParseEnvelopeCurveData(reader));
    }
    else if (FourCC('PEDU') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::EmissionDuration, reader->ReadFloat());
    }
    else if (FourCC('PLPE') == fourcc)
    {
        this->emitterAttrs.SetBool(EmitterAttrs::Looping, (1 == reader->ReadInt()));
    }
    else if (FourCC('PACD') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::ActivityDistance, reader->ReadFloat());
    }
    else if (FourCC('PROF') == fourcc)
    {
        this->emitterAttrs.SetBool(EmitterAttrs::RenderOldestFirst, (1 == reader->ReadInt()));
    }
    else if (FourCC('PBBO') == fourcc)
    {
        // ATTENTION, this is not set correctly to the right shader instance!!!!
        this->emitterAttrs.SetBool(EmitterAttrs::Billboard, (1 == reader->ReadInt()));
    }
    else if (FourCC('PRMN') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::StartRotationMin, reader->ReadFloat());
    }
    else if (FourCC('PRMX') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::StartRotationMax, reader->ReadFloat());
    }
    else if (FourCC('PGRV') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::Gravity, reader->ReadFloat());
    }
    else if (FourCC('PSTC') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::ParticleStretch, reader->ReadFloat());
    }
    else if (FourCC('PTTX') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::TextureTile, (float)reader->ReadInt());
    }
    else if (FourCC('PSTS') == fourcc)
    {
        this->emitterAttrs.SetBool(EmitterAttrs::StretchToStart, (1 == reader->ReadInt()));
    }
    else if (FourCC('PVRM') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::VelocityRandomize, reader->ReadFloat());
    }
    else if (FourCC('PRRM') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::RotationRandomize, reader->ReadFloat());
    }
    else if (FourCC('PSRM') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::SizeRandomize, reader->ReadFloat());
    }
    else if (FourCC('PPCT') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::PrecalcTime, reader->ReadFloat());
    }
    else if (FourCC('PRRD') == fourcc)
    {
        this->emitterAttrs.SetBool(EmitterAttrs::RandomizeRotation, (1 == reader->ReadInt()));
    }
    else if (FourCC('PSDL') == fourcc)
    {
        this->emitterAttrs.SetInt(EmitterAttrs::StretchDetail, reader->ReadInt());
    }
    else if (FourCC('PVAF') == fourcc)
    {
        this->emitterAttrs.SetBool(EmitterAttrs::ViewAngleFade, (1 == reader->ReadInt()));
    }
    else if (FourCC('PVAP') == fourcc)
    {
        this->emitterAttrs.SetInt(EmitterAttrs::AnimPhases, reader->ReadInt());
    }
    else if (FourCC('PDEL') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::StartDelay, reader->ReadFloat());
    }
    else if (FourCC('PDPS') == fourcc)
    {
        this->emitterAttrs.SetFloat(EmitterAttrs::PhasesPerSecond, reader->ReadFloat());
    }
    else if (FourCC('WIDR') == fourcc)
    {
        this->emitterAttrs.SetVec4(EmitterAttrs::WindDirection, reader->ReadVec4());    
    }
    else if (FourCC('MESH') == fourcc)
    {
        this->meshResId = reader->ReadString();
        this->tag = tag;
        this->UpdateMeshResource(this->meshResId);
    }
    else if (FourCC('PGRI') == fourcc)
    {
        this->primGroupIndex = reader->ReadInt();
    }
    else
    {
        retval = ShaderStateNode::Load(fourcc, tag, reader, immediate);
    }   
    return retval;    
}

} // namespace Particles
