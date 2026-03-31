//------------------------------------------------------------------------------
//  particleloader.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "particleloader.h"
#include "io/jsonreader.h"
#include "particles/emitterattrs.h"
#include "particleresource.h"
#include "resources/resourceserver.h"

namespace IO
{

//------------------------------------------------------------------------------
/**
*/
template <>
void
JsonReader::Get<Particles::EnvelopeCurve>(Particles::EnvelopeCurve& ret, const char* attr)
{
    ret = Particles::EnvelopeCurve();

    const pjson::value_variant* node = this->GetChild(attr);
    if (node->is_array())
    {
        float values[4];
        float limits[2];
        float keyPos0, keyPos1, frequency, amplitude;
        int mod;
        this->Get<float>(values[0]);
        this->Get<float>(values[1]);
        this->Get<float>(values[2]);
        this->Get<float>(values[3]);
        this->Get<float>(limits[0]);
        this->Get<float>(limits[1]);
        this->Get<float>(keyPos0);
        this->Get<float>(keyPos1);
        this->Get<float>(frequency);
        this->Get<float>(amplitude);
        this->Get<int>(mod);
        ret.SetValues(values[0], values[1], values[2], values[3]);
        ret.SetLimits(limits[0], limits[1]);
        ret.SetKeyPos0(keyPos0);
        ret.SetKeyPos1(keyPos1);
        ret.SetFrequency(frequency);
        ret.SetAmplitude(amplitude);
        ret.SetModFunc(mod);
    }
}


} // namespace IO
namespace Particles
{

__ImplementClass(ParticleLoader, 'PALD', Resources::ResourceLoader)

//------------------------------------------------------------------------------
/**
*/
void
ParticleLoader::Setup()
{
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
ParticleLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);
    Resources::ResourceLoader::ResourceInitOutput ret;

    ParticleResourceId id = particleResourceAllocator.Alloc();
    if (reader->Open())
    {
        if (!reader->HasNode("emitters"))
        {
            n_error("ParticleLoader: '%s' is not a valid particle!", stream->GetURI().AsString().AsCharPtr());
            return ret;
        }

        ParticleEmitter resource;
        reader->SetToNode("emitters");
        
        if (reader->SetToFirstChild()) do
        {
            Particles::EmitterAttrs attrs;
            resource.meshes.Append(Resources::CreateResource(reader->GetString("mesh"), job.tag));
            resource.albedo = Resources::CreateResource(reader->GetOptString("albedo", "systex:white.dds"), job.tag));
            resource.material = Resources::CreateResource(reader->GetOptString("material", "systex:default_material.dds"), job.tag));
            resource.normals = Resources::CreateResource(reader->GetOptString("normals", "systex:nobump.dds"), job.tag));

            reader->SetToNode("floats");
            for (uint i = 0; i < Particles::EmitterAttrs::FloatAttr::NumFloatAttrs; i++)
            {
                attrs.SetFloat((Particles::EmitterAttrs::FloatAttr)i, reader->GetFloat(Particles::FloatAttrNames[i]));
            }
            reader->SetToParent();

            reader->SetToNode("bools");
            for (uint i = 0; i < Particles::EmitterAttrs::BoolAttr::NumBoolAttrs; i++)
            {
                attrs.SetBool((Particles::EmitterAttrs::BoolAttr)i, reader->GetBool(Particles::BoolAttrNames[i]));
            }
            reader->SetToParent();

            reader->SetToNode("ints");
            for (uint i = 0; i < Particles::EmitterAttrs::IntAttr::NumIntAttrs; i++)
            {
                attrs.SetInt((Particles::EmitterAttrs::IntAttr)i, reader->GetInt(Particles::IntAttrNames[i]));
            }
            reader->SetToParent();

            reader->SetToNode("vecs");
            for (uint i = 0; i < Particles::EmitterAttrs::Float4Attr::NumFloat4Attrs; i++)
            {
                attrs.SetVec4((Particles::EmitterAttrs::Float4Attr)i, reader->GetVec4(Particles::Float4AttrNames[i]));
            }
            reader->SetToParent();

            reader->SetToNode("curves");
            for (uint i = 0; i < Particles::EmitterAttrs::EnvelopeAttr::NumEnvelopeAttrs; i++)
            {
                Particles::EnvelopeCurve curve;
                reader->Get<Particles::EnvelopeCurve>(curve, Particles::EnvelopeAttrNames[i]);
                attrs.SetEnvelope((Particles::EmitterAttrs::EnvelopeAttr)i, curve);
            }
            reader->SetToParent();
            resource.emitters.Append(attrs);
        } while (reader->SetToNextChild());
    }

    return ResourceInitOutput();
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleLoader::Unload(const Resources::ResourceId id)
{
}

} // namespace Particles
