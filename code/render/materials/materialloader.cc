//------------------------------------------------------------------------------
//  materialloader.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "materialloader.h"
#include "io/bxmlreader.h"
#include "resources/resourceserver.h"

#include "materials/gpulang/material_interfaces.h"
namespace Materials
{

using LoaderFunc = void(*)(Ptr<IO::BXmlReader>, Materials::MaterialId, Util::StringAtom);
Util::Dictionary<MaterialTemplatesGPULang::MaterialProperties, LoaderFunc> LoaderMap;

template <typename INTERFACE_TYPE>
struct MaterialBuffer
{
    Ids::IdGenerationPool pool;
    CoreGraphics::BufferCreateInfo hostBufferCreateInfo, deviceBufferCreateInfo;
    char* hostBufferData;
    CoreGraphics::BufferId hostBuffer, deviceBuffer;
    uint64_t deviceAddress;
    Util::PinnedArray<0xFFFF, INTERFACE_TYPE> cpuBuffer;
    bool dirty;

    MaterialBuffer(const char* name)
        : hostBufferData(nullptr)
        , hostBuffer(CoreGraphics::InvalidBufferId)
        , deviceBuffer(CoreGraphics::InvalidBufferId)
        , dirty(false)
    {
        this->hostBufferCreateInfo.name = Util::String::Sprintf("%s Host Buffer", name);
        this->hostBufferCreateInfo.usageFlags = CoreGraphics::BufferUsage::TransferSource;
        this->hostBufferCreateInfo.queueSupport = CoreGraphics::GraphicsQueueSupport;
        this->hostBufferCreateInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;

        this->deviceBufferCreateInfo.name = Util::String::Sprintf("%s Device Buffer", name);
        this->deviceBufferCreateInfo.usageFlags = CoreGraphics::BufferUsage::TransferDestination | CoreGraphics::BufferUsage::ShaderAddress | CoreGraphics::BufferUsage::ReadWrite;
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
                this->hostBufferData = (char*)CoreGraphics::BufferMap(this->hostBuffer);
                this->deviceBuffer = CoreGraphics::CreateBuffer(this->deviceBufferCreateInfo);
                this->deviceAddress = CoreGraphics::BufferGetDeviceAddress(this->deviceBuffer);
                this->dirty = true;
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
        // Copy to host mapped buffer
        Memory::Copy(this->cpuBuffer.ConstBegin(), this->hostBufferData, sizeof(INTERFACE_TYPE) * this->cpuBuffer.Size());

        // Copy to device buffer
        CoreGraphics::BufferCopy copy;
        copy.offset = 0;
        CoreGraphics::CmdCopy(id, this->hostBuffer, { copy }, this->deviceBuffer, { copy }, sizeof(INTERFACE_TYPE) * this->cpuBuffer.Size());
    }
};

#define MATERIAL_LIST \
X(BRDFMaterial) \
X(BSDFMaterial) \
X(GLTFMaterial) \
X(UnlitMaterial) \
X(BlendAddMaterial) \
X(SkyboxMaterial) \
X(TerrainMaterial) \

#define PROPERTIES_LIST \
X(BRDF) \
X(BSDF) \
X(GLTF) \
X(Unlit) \
X(BlendAdd) \
X(Skybox) \
X(Terrain) \

struct
{
#define X(x) MaterialBuffer<MaterialInterfaces::x> x##s = #x;
    MATERIAL_LIST
#undef X

    MaterialInterfaces::MaterialPointers::STRUCT bindings;
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

} materialLoaderState;

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
            materialLoaderState.dirtySet.bits = BindlessBufferDirtyBits::All;
            dirtyFlag = true;
        },
        [&handle, &dirtyFlag](Resources::ResourceId rid) mutable
        {
            CoreGraphics::TextureIdLock _0(rid);
            handle = CoreGraphics::TextureGetBindlessHandle(rid);
            materialLoaderState.dirtySet.bits = BindlessBufferDirtyBits::All;
            dirtyFlag = true;
        });
        handle = CoreGraphics::TextureGetBindlessHandle(tmp);
        reader->SetToParent();
    }
    else
    {
        CoreGraphics::TextureIdLock _0(def);
        handle = CoreGraphics::TextureGetBindlessHandle(def);
    }
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
        reader->SetToParent();
    }
    else
    {
        def.store(value);
    }
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
        reader->SetToParent();
    }
    else
    {
        def.store(value);
    }
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
        reader->SetToParent();
    }
    else
    {
        value = def;
    }
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
    MaterialTemplatesGPULang::SetupMaterialTemplates();

    // Create binding buffer
    CoreGraphics::BufferCreateInfo materialBindingInfo;
    materialBindingInfo.name = "Material Binding Buffer";
    materialBindingInfo.byteSize = sizeof(MaterialInterfaces::MaterialPointers::STRUCT);
    materialBindingInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    materialBindingInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    materialBindingInfo.queueSupport = CoreGraphics::GraphicsQueueSupport | CoreGraphics::ComputeQueueSupport;
    materialLoaderState.materialBindingBuffer.Create(materialBindingInfo);

#define ALLOC_MATERIAL(x) \
    Ids::Id32 id = materialLoaderState.x##s.Alloc();\
    if (materialLoaderState.bindings.x##s != materialLoaderState.x##s.deviceAddress)\
    {\
        materialLoaderState.bindings.x##s = materialLoaderState.x##s.deviceAddress;\
        materialLoaderState.dirtySet.bits = 0x3;\
    }\
    MaterialInterfaces::x& material = materialLoaderState.x##s.Get(id);\
    materialLoaderState.x##s.dirty = true;\

#if WITH_NEBULA_EDITOR
    #define ALLOC_AND_BIND_MATERIAL(x) \
        Ids::Id32 id = materialLoaderState.x##s.Alloc();\
        if (materialLoaderState.bindings.x##s != materialLoaderState.x##s.deviceAddress)\
        {\
            materialLoaderState.bindings.x##s = materialLoaderState.x##s.deviceAddress;\
            materialLoaderState.dirtySet.bits = 0x3;\
        }\
        MaterialInterfaces::x& material = materialLoaderState.x##s.Get(id);\
        materialLoaderState.x##s.dirty = true;\
        Materials::MaterialSetBufferBinding(mat, id);\
        Materials::MaterialBindlessForEditor(mat, (char*)&material, &materialLoaderState.x##s.dirty);
#else
    #define ALLOC_AND_BIND_MATERIAL(x, editor) \
        Ids::Id32 id = materialLoaderState.x##s.Alloc();\
        if (materialLoaderState.bindings.x##s != materialLoaderState.x##s.deviceAddress)\
        {\
            materialLoaderState.bindings.x##s = materialLoaderState.x##s.deviceAddress;\
            materialLoaderState.dirtySet.bits = 0x3;\
        }\
        MaterialInterfaces::x& material = materialLoaderState.x##s.Get(id);\
        materialLoaderState.x##s.dirty = true;\
        Materials::MaterialSetBufferBinding(mat, id);
#endif

    auto gltfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_AND_BIND_MATERIAL(GLTFMaterial);
        LoadTexture(reader, CoreGraphics::White2D, "baseColorTexture", tag.Value(), material.baseColorTexture, materialLoaderState.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "normalTexture", tag.Value(), material.normalTexture, materialLoaderState.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "metallicRoughnessTexture", tag.Value(), material.metallicRoughnessTexture, materialLoaderState.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "emissiveTexture", tag.Value(), material.emissiveTexture, materialLoaderState.GLTFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "occlusionTexture", tag.Value(), material.occlusionTexture, materialLoaderState.GLTFMaterials.dirty);
        LoadVec4(reader, "baseColorFactor", material.baseColorFactor, Math::vec4(1));
        LoadVec4(reader, "emissiveFactor", material.emissiveFactor, Math::vec4(1));
        LoadFloat(reader, "metallicFactor", material.metallicFactor, 1);
        LoadFloat(reader, "roughnessFactor", material.roughnessFactor, 1);
        LoadFloat(reader, "normalScale", material.normalScale, 1);
        LoadFloat(reader, "alphaCutoff", material.alphaCutoff, 1);
    };
    LoaderMap.Add(MaterialTemplatesGPULang::MaterialProperties::GLTF, gltfLoader);

    auto brdfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_AND_BIND_MATERIAL(BRDFMaterial);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, materialLoaderState.BRDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "ParameterMap", tag.Value(), material.ParameterMap, materialLoaderState.BRDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "NormalMap", tag.Value(), material.NormalMap, materialLoaderState.BRDFMaterials.dirty);
        LoadVec4(reader, "MatAlbedoIntensity", material.MatAlbedoIntensity, Math::vec4(1));
        LoadVec4(reader, "MatSpecularIntensity", material.MatSpecularIntensity, Math::vec4(1));
        LoadFloat(reader, "MatRoughnessIntensity", material.MatRoughnessIntensity, 1);
        LoadFloat(reader, "MatMetallicIntensity", material.MatMetallicIntensity, 1);
        LoadFloat(reader, "AlphaSensitivity", material.AlphaSensitivity, 1);
        LoadFloat(reader, "AlphaBlendFactor", material.AlphaBlendFactor, 0);
    };
    LoaderMap.Add(MaterialTemplatesGPULang::MaterialProperties::BRDF, brdfLoader);

    auto bsdfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_AND_BIND_MATERIAL(BSDFMaterial);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, materialLoaderState.BSDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Black2D, "ParameterMap", tag.Value(), material.ParameterMap, materialLoaderState.BSDFMaterials.dirty);
        LoadTexture(reader, CoreGraphics::Green2D, "NormalMap", tag.Value(), material.NormalMap, materialLoaderState.BSDFMaterials.dirty);
        LoadVec4(reader, "MatAlbedoIntensity", material.MatAlbedoIntensity, Math::vec4(1));
        LoadVec4(reader, "MatSpecularIntensity", material.MatSpecularIntensity, Math::vec4(1));
        LoadFloat(reader, "MatRoughnessIntensity", material.MatRoughnessIntensity, 1);
        LoadFloat(reader, "MatMetallicIntensity", material.MatMetallicIntensity, 1);
        LoadFloat(reader, "AlphaSensitivity", material.AlphaSensitivity, 1);
        LoadFloat(reader, "AlphaBlendFactor", material.AlphaBlendFactor, 0);
        LoadFloat(reader, "Transmission", material.Transmission, 0);
    };
    LoaderMap.Add(MaterialTemplatesGPULang::MaterialProperties::BSDF, bsdfLoader);

    auto unlitLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_AND_BIND_MATERIAL(UnlitMaterial);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, materialLoaderState.UnlitMaterials.dirty);
    };
    LoaderMap.Add(MaterialTemplatesGPULang::MaterialProperties::Unlit, unlitLoader);

    auto blendAddLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_AND_BIND_MATERIAL(BlendAddMaterial);
        LoadTexture(reader, CoreGraphics::White2D, "AlbedoMap", tag.Value(), material.AlbedoMap, materialLoaderState.BlendAddMaterials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer2", tag.Value(), material.Layer2, materialLoaderState.BlendAddMaterials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer3", tag.Value(), material.Layer3, materialLoaderState.BlendAddMaterials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "Layer4", tag.Value(), material.Layer4, materialLoaderState.BlendAddMaterials.dirty);
    };
    LoaderMap.Add(MaterialTemplatesGPULang::MaterialProperties::BlendAdd, blendAddLoader);

    auto skyboxLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        ALLOC_AND_BIND_MATERIAL(SkyboxMaterial);
        LoadTexture(reader, CoreGraphics::White2D, "SkyLayer1", tag.Value(), material.SkyLayer1, materialLoaderState.SkyboxMaterials.dirty);
        LoadTexture(reader, CoreGraphics::White2D, "SkyLayer2", tag.Value(), material.SkyLayer2, materialLoaderState.SkyboxMaterials.dirty);
        LoadFloat(reader, "SkyBlendFactor", material.SkyBlendFactor, 0);
        LoadFloat(reader, "SkyRotationFactor", material.SkyRotationFactor, 0);
        LoadFloat(reader, "Contrast", material.Contrast, 1);
        LoadFloat(reader, "Brightness", material.Brightness, 1);
    };
    LoaderMap.Add(MaterialTemplatesGPULang::MaterialProperties::Skybox, skyboxLoader);

    // never forget to run this
    ResourceLoader::Setup();
}

//------------------------------------------------------------------------------
/**
*/
void
LoadMaterialParameter(Ptr<IO::BXmlReader> reader, Util::StringAtom name, const MaterialTemplatesGPULang::Entry* entry, const Materials::MaterialId id, bool immediate)
{
    IndexT valueIndex = entry->valuesByHash.FindIndex(name.StringHashCode());
    IndexT textureIndex = entry->texturesByHash.FindIndex(name.StringHashCode());
    if (valueIndex != InvalidIndex)
    {
        const MaterialTemplatesGPULang::MaterialTemplateValue* value = entry->valuesByHash.ValueAtIndex(valueIndex);

        // Get value from material, if the type doesn't match the template, we'll pick the template value
        switch (value->type)
        {
            case MaterialTemplatesGPULang::MaterialTemplateValue::Scalar:
            {
                float f = reader->GetOptFloat("value", value->data.f);
                MaterialSetConstant(id, &f, sizeof(f), value->offset);
                break;
            }
            case MaterialTemplatesGPULang::MaterialTemplateValue::Bool:
            {
                bool b = reader->GetOptBool("value", value->data.b);
                MaterialSetConstant(id, &b, sizeof(b), value->offset);
                break;
            }
            case MaterialTemplatesGPULang::MaterialTemplateValue::Vec2:
            {
                Math::float2 f2 = reader->GetOptVec2("value", value->data.f2);
                MaterialSetConstant(id, &f2, sizeof(f2), value->offset);
                break;
            }
            case MaterialTemplatesGPULang::MaterialTemplateValue::Vec3:
            case MaterialTemplatesGPULang::MaterialTemplateValue::Vec4:
            case MaterialTemplatesGPULang::MaterialTemplateValue::Color:
            {
                Math::float4 f4 = reader->GetOptVec4("value", value->data.f4);
                MaterialSetConstant(id, &f4, sizeof(f4), value->offset);
                break;
            }
            default: break;
        }
    }
    if (textureIndex != InvalidIndex)
    {
        const MaterialTemplatesGPULang::MaterialTemplateTexture* value = entry->texturesByHash.ValueAtIndex(textureIndex);

        Util::String path = reader->GetOptString("value", value->resource);
        Resources::ResourceId tex;

        tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, "materials",
            [id, value](Resources::ResourceId rid) mutable
        {
            CoreGraphics::TextureIdLock _0(rid);
            if (value->bindlessOffset != 0xFFFFFFFF)
            {
                uint handle = CoreGraphics::TextureGetBindlessHandle(rid);
                MaterialSetTextureBindless(id, value->hashedName, handle, value->bindlessOffset, rid);
            }
            else
            {
                MaterialSetTexture(id, value->hashedName, rid);
            }
            MaterialAddLODTexture(id, rid);
        },
            [id, value](Resources::ResourceId rid) mutable
        {
            CoreGraphics::TextureIdLock _0(rid);
            if (value->bindlessOffset != 0xFFFFFFFF)
            {
                uint handle = CoreGraphics::TextureGetBindlessHandle(rid);
                MaterialSetTextureBindless(id, value->hashedName, handle, value->bindlessOffset, rid);
            }
            else
            {
                MaterialSetTexture(id, value->hashedName, rid);
            }
        }, immediate);

        CoreGraphics::TextureIdLock _0(tex);
        if (value->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(tex);
            MaterialSetTextureBindless(id, value->hashedName, handle, value->bindlessOffset, tex);
        }
        else
        {
            MaterialSetTexture(id, value->hashedName, tex);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
MaterialLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    Ptr<IO::BXmlReader> reader = IO::BXmlReader::Create();
    reader->SetStream(stream);
    Resources::ResourceLoader::ResourceInitOutput ret;

    if (reader->Open())
    {
        // make sure it's a valid frame shader file
        if (!reader->HasNode("/Nebula/Surface"))
        {
            n_error("MaterialLoader: '%s' is not a valid surface!", stream->GetURI().AsString().AsCharPtr());
            return ret;
        }

        // send to first node
        reader->SetToNode("/Nebula/Surface");

        // Get template
        Util::String templateName = reader->GetString("template");
        uint templateHash = templateName.HashCode();
        IndexT templateIndex = MaterialTemplatesGPULang::Lookup.FindIndex(templateHash);
        n_assert_fmt(templateIndex != InvalidIndex, "Unknown material template '%s'", templateName.AsCharPtr());
        const MaterialTemplatesGPULang::Entry* materialTemplate = MaterialTemplatesGPULang::Lookup.ValueAtIndex(templateIndex);
        MaterialId id = CreateMaterial(materialTemplate, job.name);
        ret.id = id;
        materialLoaderState.dirtySet.bits = BindlessBufferDirtyBits::All;

        // New material upload system, the defaults and types can be discarded
        if (reader->SetToFirstChild("Params"))
        {
            // This is the new-new system, using material buffers and is fully raytracing compatible
            IndexT loaderIndex = LoaderMap.FindIndex((MaterialTemplatesGPULang::MaterialProperties)materialTemplate->properties);
            if (loaderIndex != InvalidIndex)
            {
                auto loader = LoaderMap.ValueAtIndex(loaderIndex);
                loader(reader, id, job.tag);
            }

            // This is the legacy material system loaded with the new surface format
            if (reader->SetToFirstChild()) do
            {
                Util::StringAtom paramName = reader->GetCurrentNodeName();
                LoadMaterialParameter(reader, paramName, materialTemplate, id, job.immediate);
            }
            while (reader->SetToNextChild());
        }
        else
        {
            // Set invalid buffer binding
            MaterialSetBufferBinding(id, -1);

            // Legacy loading with legacy format where each param is a node called Param
            if (reader->SetToFirstChild("Param")) do
            {
                Util::StringAtom paramName = reader->GetString("name");
                LoadMaterialParameter(reader, paramName, materialTemplate, id, job.immediate);
            }
            while (reader->SetToNextChild("Param"));
        }
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void*
MaterialLoader::AllocateConstantMemory(SizeT size)
{
    return materialLoaderState.variantAllocator.Alloc(size);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::FlushMaterialBuffers(const CoreGraphics::CmdBufferId cmdBuf, const CoreGraphics::QueueType queue)
{
    bool anyDirty = false;
    uint bits = queue == CoreGraphics::GraphicsQueueType ? 0x1 : 0x2;
    CoreGraphics::PipelineStage sourceStage = queue == CoreGraphics::GraphicsQueueType ? CoreGraphics::PipelineStage::AllShadersRead : CoreGraphics::PipelineStage::ComputeShaderRead;
    Util::Array<CoreGraphics::BufferBarrierInfo> hostBuffers, deviceBuffers;
#define X(x)\
    if (materialLoaderState.x##s.dirty)\
    {\
        if (materialLoaderState.x##s.hostBuffer != CoreGraphics::InvalidBufferId) \
        { \
            hostBuffers.Append({.buf = materialLoaderState.x##s.hostBuffer});\
            deviceBuffers.Append({.buf = materialLoaderState.x##s.deviceBuffer});\
        }\
        anyDirty = true;\
    }
    MATERIAL_LIST
#undef X

    if (materialLoaderState.dirtySet.bits & bits)
    {
        CoreGraphics::BufferUpdate(materialLoaderState.materialBindingBuffer.HostBuffer(), materialLoaderState.bindings, 0);
        hostBuffers.Append({ .buf = materialLoaderState.materialBindingBuffer.HostBuffer() });
        deviceBuffers.Append({ .buf = materialLoaderState.materialBindingBuffer.DeviceBuffer() });
        anyDirty = true;
    }

    if (anyDirty)
    {
        CoreGraphics::BarrierPush(
            cmdBuf
            , CoreGraphics::PipelineStage::HostWrite
            , CoreGraphics::PipelineStage::TransferRead
            , CoreGraphics::BarrierDomain::Global
            , hostBuffers
        );

        CoreGraphics::BarrierPush(
            cmdBuf
            , sourceStage
            , CoreGraphics::PipelineStage::TransferWrite
            , CoreGraphics::BarrierDomain::Global
            , deviceBuffers
        );

#define X(x) \
        if (materialLoaderState.x##s.dirty)\
        {\
            materialLoaderState.x##s.Flush(cmdBuf, queue);\
            materialLoaderState.x##s.dirty = false;\
        }
        MATERIAL_LIST
#undef X

        if (materialLoaderState.dirtySet.bits & bits)
        {
            materialLoaderState.materialBindingBuffer.Flush(cmdBuf, sizeof(MaterialInterfaces::MaterialPointers::STRUCT));
            materialLoaderState.dirtySet.bits &= ~bits;
        }

        CoreGraphics::BarrierPop(cmdBuf);
        CoreGraphics::BarrierPop(cmdBuf);
    }
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
MaterialLoader::GetMaterialBindingBuffer()
{
    return materialLoaderState.materialBindingBuffer.DeviceBuffer();
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::BufferId
MaterialLoader::GetMaterialBuffer(const MaterialTemplatesGPULang::MaterialProperties type)
{
    switch (type)
    {
#define X(x) \
    case MaterialTemplatesGPULang::MaterialProperties::x:\
    return materialLoaderState.x##Materials.deviceBuffer;\

    PROPERTIES_LIST
#undef X
    default: break;
    }
    return CoreGraphics::InvalidBufferId;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MaterialLoader::RegisterTerrainMaterial(const MaterialInterfaces::TerrainMaterial& terrain)
{
    ALLOC_MATERIAL(TerrainMaterial);
    material.LowresAlbedoFallback = terrain.LowresAlbedoFallback;
    material.LowresMaterialFallback = terrain.LowresMaterialFallback;
    material.LowresNormalFallback = terrain.LowresNormalFallback;
    materialLoaderState.dirtySet.bits = BindlessBufferDirtyBits::All;
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
