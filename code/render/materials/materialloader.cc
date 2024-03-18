//------------------------------------------------------------------------------
//  materialloader.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "materialloader.h"
#include "io/bxmlreader.h"
#include "resources/resourceserver.h"
#include "materials/materialtemplates.h"

#include "system_shaders/material_interface.h"

#include "materials/base.h"
namespace Materials
{

template <typename INTERFACE_TYPE>
struct MaterialBuffer
{
    Ids::IdGenerationPool pool;
    CoreGraphics::BufferCreateInfo hostBufferCreateInfo, deviceBufferCreateInfo;
    INTERFACE_TYPE* hostBufferData;
    CoreGraphics::BufferId hostBuffer, deviceBuffer;
    Util::PinnedArray<0xFFFF, INTERFACE_TYPE> cpuBuffer;
    bool dirty;

    MaterialBuffer(const char* name)
        : dirty(false)
        , hostBuffer(CoreGraphics::InvalidBufferId)
        , deviceBuffer(CoreGraphics::InvalidBufferId)
        , hostBufferData(nullptr)
    {
        this->hostBufferCreateInfo.name = Util::String::Sprintf("%s Host Buffer", name);
        this->hostBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::TransferBufferSource;
        this->hostBufferCreateInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
        this->hostBufferCreateInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;

        this->deviceBufferCreateInfo.name = Util::String::Sprintf("%s Device Buffer", name);
        this->deviceBufferCreateInfo.usageFlags = CoreGraphics::BufferUsageFlag::TransferBufferDestination | CoreGraphics::BufferUsageFlag::ShaderAddress | CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
        this->deviceBufferCreateInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
        this->deviceBufferCreateInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    }

    /// Allocate a new material
    Ids::Id32 Alloc()
    {
        Ids::Id32 ret;
        if (this->pool.Allocate(ret))
        {
            SizeT oldCapacity = this->cpuBuffer.Capacity();
            this->cpuBuffer.Emplace();
            SizeT newCapacity = this->cpuBuffer.Capacity();

            // If capacity has changed, we need to reallocate our buffers
            if (oldCapacity != newCapacity)
            {
                // Destroy old buffers
                if (this->hostBuffer != CoreGraphics::InvalidBufferId)
                {
                    CoreGraphics::DestroyBuffer(this->hostBuffer);
                    CoreGraphics::DestroyBuffer(this->deviceBuffer);
                }

                // Update size to fit new capacity
                this->hostBufferCreateInfo.byteSize = sizeof(INTERFACE_TYPE) * newCapacity;
                this->deviceBufferCreateInfo.byteSize = sizeof(INTERFACE_TYPE) * newCapacity;

                // Create new buffers
                this->hostBuffer = CoreGraphics::CreateBuffer(this->hostBufferCreateInfo);
                this->hostBufferData = (INTERFACE_TYPE*)CoreGraphics::BufferMap(this->hostBuffer);
                this->deviceBuffer = CoreGraphics::CreateBuffer(this->deviceBufferCreateInfo);
            }
        }
        return ret;
    }

    /// Get material instance for updating
    INTERFACE_TYPE& Get(Ids::Id32 id)
    {
        n_assert(this->pool.IsValid(id));
        return this->cpuBuffer[Ids::Index(id)];
    }

    /// Flush
    void Flush(const CoreGraphics::CmdBufferId id, const CoreGraphics::QueueType queue)
    {
        // Copy from cpu buffer to host buffer
        if (this->dirty)
        {
            CoreGraphics::PipelineStage sourceStage = queue == CoreGraphics::GraphicsQueueType ? CoreGraphics::PipelineStage::AllShadersRead : CoreGraphics::PipelineStage::ComputeShaderRead;
            Memory::CopyElements(this->cpuBuffer.ConstBegin(), this->hostBufferData, this->cpuBuffer.Size());

            CoreGraphics::BarrierPush(
                id
                , CoreGraphics::PipelineStage::HostWrite
                , CoreGraphics::PipelineStage::TransferRead
                , CoreGraphics::BarrierDomain::Global
                , {
                        CoreGraphics::BufferBarrierInfo
                    {
                            .buf = this->hostBuffer,
                            .subres = CoreGraphics::BufferSubresourceInfo{}
                    }
                  }
            );
            CoreGraphics::BarrierPush(
                id
                , sourceStage
                , CoreGraphics::PipelineStage::TransferWrite
                , CoreGraphics::BarrierDomain::Global
                , {
                        CoreGraphics::BufferBarrierInfo
                    {
                            .buf = this->deviceBuffer,
                            .subres = CoreGraphics::BufferSubresourceInfo{}
                    }
                }
            );

            // Now copy to GPU
            CoreGraphics::BufferCopy copy;
            copy.offset = 0;
            CoreGraphics::CmdCopy(id, this->hostBuffer, { copy }, this->deviceBuffer, { copy }, sizeof(INTERFACE_TYPE) * this->cpuBuffer.Size());

            CoreGraphics::BarrierPop(id);
            CoreGraphics::BarrierPop(id);
            this->dirty = false;
        }
    }
};

#define MATERIAL_LIST \
X(BRDFMaterial) \
X(BSDFMaterial) \
X(GLTFMaterial) \
X(UnlitMaterial) \
X(Unlit2Material) \
X(Unlit3Material) \
X(Unlit4Material) \
X(SkyboxMaterial) \
X(LegacyMaterial) \
X(TerrainMaterial) \

#define PROPERTIES_LIST \
X(BRDF) \
X(BSDF) \
X(GLTF) \
X(Unlit) \
X(Unlit2) \
X(Unlit3) \
X(Unlit4) \
X(Skybox) \
X(Legacy) \
X(Terrain) \

struct
{
#define X(x) MaterialBuffer<MaterialInterface::x> x##s = #x;
    MATERIAL_LIST
#undef X

    MaterialInterface::MaterialBindings bindings;
    CoreGraphics::BufferWithStaging materialBindingBuffer;

    Threading::CriticalSection variantAllocatorLock;
    Memory::ArenaAllocator<4096> variantAllocator;

    union DirtySet
    {
        struct
        {
            bool graphicsDirty : 1;
            bool computeDirty : 1;
        };
        int bits;
    } dirtySet;

} state;

__ImplementClass(Materials::MaterialLoader, 'MALO', Resources::ResourceLoader);

//------------------------------------------------------------------------------
/**
*/
void
LoadTexture(const Ptr<IO::BXmlReader>& reader, CoreGraphics::TextureId def, const char* name, const char* tag, uint& handle, bool& dirtyFlag)
{
    if (reader->SetToFirstChild(name))
    {
        auto tmp = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag,
        [&handle, &dirtyFlag](Resources::ResourceId rid) mutable
        {
            CoreGraphics::TextureIdLock _0(rid);
            handle = CoreGraphics::TextureGetBindlessHandle(rid);
            state.dirtySet.bits = 0x3;
            dirtyFlag = true;
        },
        [&handle, &dirtyFlag](Resources::ResourceId rid) mutable
        {
            CoreGraphics::TextureIdLock _0(rid);
            handle = CoreGraphics::TextureGetBindlessHandle(rid);
            state.dirtySet.bits = 0x3;
            dirtyFlag = true;
        });
        handle = CoreGraphics::TextureGetBindlessHandle(tmp);
    }
    else
    {
        CoreGraphics::TextureIdLock _0(def);
        handle = CoreGraphics::TextureGetBindlessHandle(def);
    }
    reader->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
void
LoadVec4(const Ptr<IO::BXmlReader>& reader, const char* name, float value[4], const Math::vec4& def)
{
    if (reader->SetToFirstChild(name))
    {
        Math::vec4 val = reader->GetVec4("value");
        val.store(value);
    }
    else
    {
        def.store(value);
    }
    reader->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
void
LoadVec3(const Ptr<IO::BXmlReader>& reader, const char* name, float value[3], const Math::vec3& def)
{
    if (reader->SetToFirstChild(name))
    {
        Math::vec4 val = reader->GetVec4("value");
        val.store3(value);
    }
    else
    {
        def.store(value);
    }
    reader->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
void
LoadFloat(const Ptr<IO::BXmlReader>& reader, const char* name, float& value, const float def)
{
    if (reader->SetToFirstChild(name))
    {
        value = reader->GetFloat("value");
    }
    else
    {
        value = def;
    }
    reader->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialLoader::Setup()
{
    this->placeholderResourceName = "syssur:placeholder.sur";
    this->failResourceName = "syssur:error.sur";

    // Run generated setup code
    MaterialTemplates::SetupMaterialTemplates();

    // Create binding buffer
    CoreGraphics::BufferCreateInfo materialBindingInfo;
    materialBindingInfo.name = "Material Binding Buffer";
    materialBindingInfo.byteSize = sizeof(MaterialInterface::MaterialBindings);
    materialBindingInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
    materialBindingInfo.usageFlags = CoreGraphics::BufferUsageFlag::ReadWriteBuffer;
    materialBindingInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
    state.materialBindingBuffer = CoreGraphics::BufferWithStaging(materialBindingInfo);

#define ALLOC_MATERIAL(x) \
    Ids::Id32 id = state.x##s.Alloc();\
    state.bindings.x##s = CoreGraphics::BufferGetDeviceAddress(state.x##s.deviceBuffer);\
    MaterialInterface::x& material = state.x##s.Get(id);\
    state.x##s.dirty = true;

    auto gltfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(GLTFMaterial);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "baseColorTexture", tag.Value(), material.baseColorTexture, state.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "normalTexture", tag.Value(), material.normalTexture, state.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "metallicRoughnessTexture", tag.Value(), material.metallicRoughnessTexture, state.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "emissiveTexture", tag.Value(), material.emissiveTexture, state.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "occlusionTexture", tag.Value(), material.occlusionTexture, state.GLTFMaterials.dirty);
        LoadVec4(reader, "baseColorFactor", material.baseColorFactor, Math::vec4(1));
        LoadVec4(reader, "emissiveFactor", material.emissiveFactor, Math::vec4(1));
        LoadFloat(reader, "metallicFactor", material.metallicFactor, 1);
        LoadFloat(reader, "roughnessFactor", material.roughnessFactor, 1);
        LoadFloat(reader, "normalScale", material.normalScale, 1);
        LoadFloat(reader, "alphaCutoff", material.alphaCutoff, 1);
    };
    this->loaderMap.Add(MaterialProperties::GLTF, gltfLoader);

    auto brdfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(BRDFMaterial);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, state.BRDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "ParameterMap", tag.Value(), material.ParameterMap, state.BRDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "NormalMap", tag.Value(), material.NormalMap, state.BRDFMaterials.dirty);
        LoadVec3(reader, "MatAlbedoIntensity", material.MatAlbedoIntensity, Math::vec3(1));
        LoadFloat(reader, "MatRoughnessIntensity", material.MatRoughnessIntensity, 1);
    };
    this->loaderMap.Add(MaterialProperties::BRDF, brdfLoader);

    auto bsdfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(BSDFMaterial);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, state.BSDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "ParameterMap", tag.Value(), material.ParameterMap, state.BSDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "NormalMap", tag.Value(), material.NormalMap, state.BSDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "AbsorptionMap", tag.Value(), material.AbsorptionMap, state.BSDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "ScatterMap", tag.Value(), material.ScatterMap, state.BSDFMaterials.dirty);
        LoadVec3(reader, "MatAlbedoIntensity", material.MatAlbedoIntensity, Math::vec3(1));
        LoadFloat(reader, "MatRoughnessIntensity", material.MatRoughnessIntensity, 1);
    };
    this->loaderMap.Add(MaterialProperties::BSDF, bsdfLoader);

    auto unlitLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(UnlitMaterial);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, state.UnlitMaterials.dirty);
    };
    this->loaderMap.Add(MaterialProperties::Unlit, unlitLoader);

    auto unlit2Loader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(Unlit2Material);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "Layer1", tag.Value(), material.Layer1, state.Unlit2Materials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer2", tag.Value(), material.Layer2, state.Unlit2Materials.dirty);
    };
    this->loaderMap.Add(MaterialProperties::Unlit2, unlit2Loader);

    auto unlit3Loader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(Unlit3Material);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "Layer1", tag.Value(), material.Layer1, state.Unlit3Materials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer2", tag.Value(), material.Layer2, state.Unlit3Materials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer3", tag.Value(), material.Layer3, state.Unlit3Materials.dirty);
    };
    this->loaderMap.Add(MaterialProperties::Unlit3, unlit3Loader);

    auto unlit4Loader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(Unlit4Material);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "Layer1", tag.Value(), material.Layer1, state.Unlit4Materials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer2", tag.Value(), material.Layer2, state.Unlit4Materials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer3", tag.Value(), material.Layer3, state.Unlit4Materials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer4", tag.Value(), material.Layer4, state.Unlit4Materials.dirty);
    };
    this->loaderMap.Add(MaterialProperties::Unlit4, unlit4Loader);

    auto skyboxLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(SkyboxMaterial);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "SkyLayer1", tag.Value(), material.SkyLayer1, state.SkyboxMaterials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "SkyLayer2", tag.Value(), material.SkyLayer2, state.SkyboxMaterials.dirty);
        LoadFloat(reader, "Contrast", material.Contrast, 1);
        LoadFloat(reader, "Brightness", material.Brightness, 1);
    };
    this->loaderMap.Add(MaterialProperties::Skybox, skyboxLoader);

    auto legacyLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_MATERIAL(LegacyMaterial);
        Materials::MaterialSetBufferBinding(mat, id);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, state.LegacyMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "ParameterMap", tag.Value(), material.ParameterMap, state.LegacyMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "NormalMap", tag.Value(), material.NormalMap, state.LegacyMaterials.dirty);
        LoadVec4(reader, "MatAlbedoIntensity", material.MatAlbedoIntensity, Math::vec4(1));
        LoadVec4(reader, "MatSpecularIntensity", material.MatSpecularIntensity, Math::vec4(1));
        LoadFloat(reader, "MatRoughnessIntensity", material.MatRoughnessIntensity, 1);
        LoadFloat(reader, "MatMetallicIntensity", material.MatMetallicIntensity, 1);
        LoadFloat(reader, "AlphaSensitivity", material.AlphaSensitivity, 1);
        LoadFloat(reader, "AlphaBlendFactor", material.AlphaBlendFactor, 1);
    };
    this->loaderMap.Add(MaterialProperties::Legacy, legacyLoader);

    n_assert_msg(this->loaderMap.Size() == (uint)MaterialProperties::REQUIRED_NUM_CONTENT_LOADERS, "Missing material loaders, please add a loader for each material in the MaterialProperties enum");

    // never forget to run this
    ResourceLoader::Setup();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
MaterialLoader::InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    Ptr<IO::BXmlReader> reader = IO::BXmlReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        // make sure it's a valid frame shader file
        if (!reader->HasNode("/Nebula/Surface"))
        {
            n_error("MaterialLoader: '%s' is not a valid surface!", stream->GetURI().AsString().AsCharPtr());
            return Resources::InvalidResourceUnknownId;
        }

        // send to first node
        reader->SetToNode("/Nebula/Surface");

        // Get template
        Util::String templateName = reader->GetString("template");
        uint templateHash = templateName.HashCode();
        IndexT templateIndex = MaterialTemplates::Lookup.FindIndex(templateHash);
        n_assert_fmt(templateIndex != InvalidIndex, "Unknown material template '%s'", templateName.AsCharPtr());
        const MaterialTemplates::Entry* materialTemplate = MaterialTemplates::Lookup.ValueAtIndex(templateIndex);

        MaterialId id = CreateMaterial(materialTemplate);

        if (reader->SetToFirstChild("Param")) do
        {
            Util::StringAtom paramName = reader->GetString("name");

            // set variant value which we will use in the surface constants
            for (IndexT i = 0; i < materialTemplate->passes.Size(); i++)
            {
                uint constantIndex = materialTemplate->constantBatchLookup[i].FindIndex(paramName.StringHashCode());
                uint textureIndex = materialTemplate->textureBatchLookup[i].FindIndex(paramName.StringHashCode());

                if (constantIndex != InvalidIndex)
                {
                    // Get constant
                    const auto constant = materialTemplate->constantsPerBatch[i][materialTemplate->constantBatchLookup[i].ValueAtIndex(constantIndex)];
                    if (constant->Valid())
                    {

                        // Create variant and allocate memory
                        state.variantAllocatorLock.Enter();
                        void* mem = state.variantAllocator.Alloc(constant->def->GetSize());
                        state.variantAllocatorLock.Leave();

                        MaterialVariant var;
                        var.mem = mem;

                        // Get value from material, if the type doesn't match the template, we'll pick the template value
                        switch (constant->def->type)
                        {
                            case MaterialTemplateValue::Scalar:
                                var.Set(reader->GetOptFloat("value", constant->def->data.f), mem);
                                break;
                            case MaterialTemplateValue::Bool:
                                var.Set(reader->GetOptBool("value", constant->def->data.b), mem);
                                break;
                            case MaterialTemplateValue::Vec2:
                                var.Set(reader->GetOptVec2("value", constant->def->data.f2), mem);
                                break;
                            case MaterialTemplateValue::Vec3:
                                var.Set(reader->GetOptVec4("value", constant->def->data.f4), mem);
                                break;
                            case MaterialTemplateValue::Vec4:
                                var.Set(reader->GetOptVec4("value", constant->def->data.f4), mem);
                                break;
                            case MaterialTemplateValue::BindlessResource:
                            {
                                Util::String path = reader->GetOptString("value", constant->def->data.resource);
                                Resources::ResourceId tex;

                                tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, tag,
                                    [id, constant, var, mem](Resources::ResourceId rid) mutable
                                {
                                    CoreGraphics::TextureIdLock _0(rid);
                                    var.Set(CoreGraphics::TextureGetBindlessHandle(rid), mem);
                                    MaterialSetConstant(id, constant, var);
                                    MaterialAddLODTexture(id, rid);
                                },
                                    [id, constant, var, mem](Resources::ResourceId rid) mutable
                                {
                                    CoreGraphics::TextureIdLock _0(rid);
                                    var.Set(CoreGraphics::TextureGetBindlessHandle(rid), mem);
                                    MaterialSetConstant(id, constant, var);
                                }, immediate);
                                CoreGraphics::TextureIdLock _0(tex);
                                var.Set(CoreGraphics::TextureGetBindlessHandle(tex), mem);

                                break;
                            }
                        }

                        // Set constant
                        MaterialSetConstant(id, constant, var);
                    }
                }
                else if (textureIndex != InvalidIndex)
                {
                    const auto texture = materialTemplate->texturesPerBatch[i][materialTemplate->textureBatchLookup[i].ValueAtIndex(textureIndex)];
                    if (texture->slot != InvalidIndex)
                    {
                        Resources::ResourceId tex = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag,
                        [id, texture](Resources::ResourceId rid)
                        {
                            CoreGraphics::TextureIdLock _0(rid);
                            MaterialSetTexture(id, texture, rid);
                            MaterialAddLODTexture(id, rid);
                        },
                        [id, texture](Resources::ResourceId rid)
                        {
                            CoreGraphics::TextureIdLock _0(rid);
                            MaterialSetTexture(id, texture, rid);
                        }, immediate);

                        CoreGraphics::TextureIdLock _0(tex);
                        MaterialSetTexture(id, texture, tex);
                    }                    
                }
            }
        }
        while (reader->SetToNextChild("Param"));

        // New material upload system, the defaults and types can be discarded
        IndexT loaderIndex = this->loaderMap.FindIndex(materialTemplate->properties);
        if (loaderIndex != InvalidIndex)
        {
            auto loader = this->loaderMap.ValueAtIndex(loaderIndex);
            reader->SetToFirstChild("Params");
            loader(reader, id, tag);
            state.dirtySet.bits = 0x3;
            reader->SetToParent();
        }

        return id;
    }
    return InvalidMaterialId;
}

//------------------------------------------------------------------------------
/**
*/
void*
MaterialLoader::AllocateConstantMemory(SizeT size)
{
    return state.variantAllocator.Alloc(size);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::FlushMaterialBuffers(const CoreGraphics::CmdBufferId id, const CoreGraphics::QueueType queue)
{
    uint bits = queue == CoreGraphics::GraphicsQueueType ? 0x1 : 0x2;
    CoreGraphics::PipelineStage sourceStage = queue == CoreGraphics::GraphicsQueueType ? CoreGraphics::PipelineStage::AllShadersRead : CoreGraphics::PipelineStage::ComputeShaderRead;
    if (state.dirtySet.bits & bits)
    {
        Util::Array<CoreGraphics::BufferBarrierInfo> hostBuffers, deviceBuffers;
#define X(x) if (state.x##s.hostBuffer != CoreGraphics::InvalidBufferId) \
            { \
                hostBuffers.Append({.buf = state.x##s.hostBuffer});\
                deviceBuffers.Append({.buf = state.x##s.deviceBuffer});\
            }
        MATERIAL_LIST
#undef X
        hostBuffers.Append({ .buf = state.materialBindingBuffer.HostBuffer() });
        deviceBuffers.Append({ .buf = state.materialBindingBuffer.DeviceBuffer() });

        CoreGraphics::BufferUpdate(state.materialBindingBuffer.HostBuffer(), state.bindings, 0);
        CoreGraphics::BarrierPush(
            id
            , CoreGraphics::PipelineStage::HostWrite
            , CoreGraphics::PipelineStage::TransferRead
            , CoreGraphics::BarrierDomain::Global
            , hostBuffers
        );

        CoreGraphics::BarrierPush(
            id
            , sourceStage
            , CoreGraphics::PipelineStage::TransferWrite
            , CoreGraphics::BarrierDomain::Global
            , deviceBuffers
        );

#define X(x) \
        state.x##s.Flush(id, queue);
        MATERIAL_LIST
#undef X

        state.materialBindingBuffer.Flush(id, sizeof(MaterialInterface::MaterialBindings));

        CoreGraphics::BarrierPop(id);
        CoreGraphics::BarrierPop(id);
        state.dirtySet.bits &= ~bits;
    }
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
MaterialLoader::GetMaterialBindingBuffer()
{
    return state.materialBindingBuffer.DeviceBuffer();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
MaterialLoader::GetMaterialBuffer(const MaterialProperties type)
{
    switch (type)
    {
#define X(x) \
    case MaterialProperties::x:\
    return state.x##Materials.deviceBuffer;\

    PROPERTIES_LIST
#undef X
    }
    return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MaterialLoader::RegisterTerrainMaterial(const MaterialInterface::TerrainMaterial& terrain)
{
    ALLOC_MATERIAL(TerrainMaterial);
    material.LowresAlbedoFallback = terrain.LowresAlbedoFallback;
    material.LowresMaterialFallback = terrain.LowresMaterialFallback;
    material.LowresNormalFallback = terrain.LowresNormalFallback;
    state.dirtySet.bits = 0x3;
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::Unload(const Resources::ResourceId id)
{
    MaterialId material = id.resource;
    DestroyMaterial(material);
}

} // namespace Materials
