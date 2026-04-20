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

CoreGraphics::MeshId DefaultEmitterMesh;

//------------------------------------------------------------------------------
/**
*/
void
ParticleLoader::Setup()
{

    struct VectorB4N
    {
        char x : 8;
        char y : 8;
        char z : 8;
        char w : 8;
    };

    VectorB4N normal;
    normal.x = 0;
    normal.y = 127;
    normal.z = 0;
    normal.w = 0;

    VectorB4N tangent;
    tangent.x = 0;
    tangent.y = 0;
    tangent.z = 127;
    tangent.w = 0;

    float vertex[] = { 0, 0, 0, 0, 0 };
    memcpy(&vertex[3], &normal, 4);
    memcpy(&vertex[4], &tangent, 4);

    Util::Array<CoreGraphics::VertexComponent> emitterComponents;
    emitterComponents.Append(CoreGraphics::VertexComponent(CoreGraphics::VertexComponent::Position, CoreGraphics::VertexComponent::Float3, 0));
    emitterComponents.Append(CoreGraphics::VertexComponent(CoreGraphics::VertexComponent::Normal, CoreGraphics::VertexComponent::Byte4N, 0));
    emitterComponents.Append(CoreGraphics::VertexComponent(CoreGraphics::VertexComponent::Tangent, CoreGraphics::VertexComponent::Byte4N, 0));
    CoreGraphics::VertexLayoutId emitterLayout = CoreGraphics::CreateVertexLayout({ .name = "Emitter"_atm, .comps = emitterComponents });

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "Single Point Particle Emitter VBO";
    vboInfo.size = 1;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(emitterLayout);
    vboInfo.mode = CoreGraphics::HostLocal;
    vboInfo.usageFlags = CoreGraphics::BufferUsage::Vertex;
    vboInfo.data = vertex;
    vboInfo.dataSize = sizeof(vertex);
    CoreGraphics::BufferId vbo = CoreGraphics::CreateBuffer(vboInfo);

    uint indices[] = { 0 };
    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.name = "Single Point Particle Emitter IBO";
    iboInfo.size = 1;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
    iboInfo.mode = CoreGraphics::HostLocal;
    iboInfo.usageFlags = CoreGraphics::BufferUsage::Index;
    iboInfo.data = indices;
    iboInfo.dataSize = sizeof(indices);
    CoreGraphics::BufferId ibo = CoreGraphics::CreateBuffer(iboInfo);

    CoreGraphics::PrimitiveGroup group;
    group.SetBaseIndex(0);
    group.SetBaseVertex(0);
    group.SetNumIndices(1);
    group.SetNumVertices(1);

    // setup single point emitter mesh
    CoreGraphics::MeshCreateInfo meshInfo;
    CoreGraphics::VertexStream stream;
    stream.vertexBuffer = vbo;
    stream.index = 0;
    meshInfo.streams.Append(stream);
    meshInfo.indexBuffer = ibo;
    meshInfo.name = "Single Point Particle Emitter Mesh";
    meshInfo.primitiveGroups.Append(group);
    meshInfo.topology = CoreGraphics::PrimitiveTopology::PointList;
    meshInfo.vertexLayout = emitterLayout;
    meshInfo.indexType = CoreGraphics::IndexType::Index16;
    DefaultEmitterMesh = CoreGraphics::CreateMesh(meshInfo);
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
            resource.materials.Append(Resources::CreateResource(reader->GetString("material"), job.tag, nullptr, nullptr, true, false));
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
            resource.attrs.Append(attrs);
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
