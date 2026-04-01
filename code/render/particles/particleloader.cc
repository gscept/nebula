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
        const pjson::value_variant_vec_t& arr = node->get_array();
        float values[4];
        float limits[2];
        float keyPos0, keyPos1, frequency, amplitude;
        int mod;
        values[0] = arr[0].as_float();
        values[1] = arr[1].as_float();
        values[2] = arr[2].as_float();
        values[3] = arr[3].as_float();
        limits[0] = arr[4].as_float();
        limits[1] = arr[5].as_float();
        keyPos0 = arr[6].as_float();
        keyPos1 = arr[7].as_float();
        frequency = arr[8].as_float();
        amplitude = arr[9].as_float();
        mod = arr[10].as_int32();
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

    if (reader->Open())
    {
        ParticleResourceId id = particleResourceAllocator.Alloc();
        if (!reader->HasNode("emitters"))
        {
            n_error("ParticleLoader: '%s' is not a valid particle!", stream->GetURI().AsString().AsCharPtr());
            return ret;
        }

        ParticleEmitters resource;
        reader->SetToNode("emitters");
        
        if (reader->SetToFirstChild()) do
        {
            Particles::EmitterAttrs attrs;
            Util::String mesh = reader->GetString("mesh");
            if (mesh.IsValid())
                resource.meshes.Append(Resources::CreateResource(mesh, job.tag));
            else
                resource.meshes.Append(Resources::InvalidResourceId);

            resource.name.Append(reader->GetOptString("name", "emitter"));
            resource.albedo.Append(Resources::CreateResource(reader->GetOptString("albedo", "systex:white.dds"), job.tag));
            resource.material.Append(Resources::CreateResource(reader->GetOptString("material", "systex:default_material.dds"), job.tag));
            resource.normals.Append(Resources::CreateResource(reader->GetOptString("normals", "systex:nobump.dds"), job.tag));
            resource.transform.Append(reader->GetOptMat4("transform", Math::mat4()));

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

        particleResourceAllocator.Set<ParticleResource_Resource>(id.id, resource);
        ret.id = id;
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
ParticleLoader::Unload(const Resources::ResourceId id)
{
}

} // namespace Particles
