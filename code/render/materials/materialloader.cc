//------------------------------------------------------------------------------
//  materialloader.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "materialloader.h"
#include "io/bxmlreader.h"
#include "resources/resourceserver.h"
#include "shaderconfigserver.h"
#include "materials/materialtemplates.h"

namespace Materials
{

struct
{
    Util::PinnedArray<0xFFFF, MaterialInterface::BRDFMaterial> BRDFMaterials;
    Util::PinnedArray<0xFFFF, MaterialInterface::BSDFMaterial> BSDFMaterials;
    Util::PinnedArray<0xFFFF, MaterialInterface::GLTFMaterial> GLTFMaterials;
    Util::PinnedArray<0xFFFF, MaterialInterface::UnlitMaterial> UnlitMaterials;
    Util::PinnedArray<0xFFFF, MaterialInterface::Unlit2Material> Unlit2Materials;
    Util::PinnedArray<0xFFFF, MaterialInterface::Unlit3Material> Unlit3Materials;
    Util::PinnedArray<0xFFFF, MaterialInterface::Unlit4Material> Unlit4Materials;
    Util::PinnedArray<0xFFFF, MaterialInterface::SkyboxMaterial> SkyboxMaterials;
} state;

__ImplementClass(Materials::MaterialLoader, 'MALO', Resources::ResourceLoader);

//------------------------------------------------------------------------------
/**
*/
void 
MaterialLoader::Setup()
{
    this->placeholderResourceName = "syssur:placeholder.sur";
    this->failResourceName = "syssur:error.sur";

#define LOAD_TEXTURE(x, def) \
            if (reader->SetToFirstChild(#x)) \
            { \
                Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag, \
                [material](Resources::ResourceId rid) mutable \
                { \
                    CoreGraphics::TextureIdLock _0(rid); \
                    material.x = CoreGraphics::TextureGetBindlessHandle(rid); \
                }, \
                [material](Resources::ResourceId rid) mutable \
                { \
                    CoreGraphics::TextureIdLock _0(rid); \
                    material.x = CoreGraphics::TextureGetBindlessHandle(rid); \
                }); \
            } \
            else \
            {\
                CoreGraphics::TextureIdLock _0(def); \
                material.x = CoreGraphics::TextureGetBindlessHandle(def); \
            }
        
#define LOAD_VEC4(x, def) \
            if (reader->SetToFirstChild(#x)) \
            { \
                Math::vec4 val = reader->GetVec4("value"); \
                val.store(material.x); \
            }  \
            else \
            { \
                Math::vec4 defaultVal(def);\
                defaultVal.store(material.x);\
            }

#define LOAD_VEC3(x, def) \
            if (reader->SetToFirstChild(#x)) \
            { \
                Math::vec4 val = reader->GetVec4("value"); \
                val.store3(material.x); \
            }  \
            else \
            { \
                Math::vec4 defaultVal(def);\
                defaultVal.store3(material.x);\
            }

#define LOAD_FLOAT(x, def) \
            if (reader->SetToFirstChild(#x)) \
            { \
                material.x = reader->GetFloat("value");\
            }  \
            else \
            { \
                material.x = def;\
            }

    auto gltfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.GLTFMaterials.Size());
        MaterialInterface::GLTFMaterial& material = state.GLTFMaterials.Emplace();
        LOAD_TEXTURE(baseColorTexture, CoreGraphics::White1D);
        LOAD_TEXTURE(normalTexture, CoreGraphics::Green2D);
        LOAD_TEXTURE(metallicRoughnessTexture, CoreGraphics::Black2D);
        LOAD_TEXTURE(emissiveTexture, CoreGraphics::Black2D);
        LOAD_TEXTURE(occlusionTexture, CoreGraphics::Black2D);
        LOAD_VEC4(baseColorFactor, 1);
        LOAD_VEC4(emissiveFactor, 1);
        LOAD_FLOAT(metallicFactor, 1);
        LOAD_FLOAT(roughnessFactor, 1);
        LOAD_FLOAT(normalScale, 1);
        LOAD_FLOAT(alphaCutoff, 1);
    };
    this->loaderMap.Add(MaterialProperties::GLTF, gltfLoader);

    auto brdfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.BRDFMaterials.Size());
        MaterialInterface::BRDFMaterial& material = state.BRDFMaterials.Emplace();
        LOAD_TEXTURE(AlbedoMap, CoreGraphics::White1D);
        LOAD_TEXTURE(ParameterMap, CoreGraphics::Black2D);
        LOAD_TEXTURE(NormalMap, CoreGraphics::Green2D);
        LOAD_VEC3(MatAlbedoIntensity, 1);
        LOAD_FLOAT(MatRoughnessIntensity, 1);
    };
    this->loaderMap.Add(MaterialProperties::BRDF, brdfLoader);

    auto bsdfLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.BSDFMaterials.Size());
        MaterialInterface::BSDFMaterial& material = state.BSDFMaterials.Emplace();
        LOAD_TEXTURE(AlbedoMap, CoreGraphics::White1D);
        LOAD_TEXTURE(ParameterMap, CoreGraphics::Black2D);
        LOAD_TEXTURE(NormalMap, CoreGraphics::Green2D);
        LOAD_TEXTURE(AbsorptionMap, CoreGraphics::Black2D);
        LOAD_TEXTURE(ScatterMap, CoreGraphics::Black2D);
        LOAD_VEC3(MatAlbedoIntensity, 1);
        LOAD_FLOAT(MatRoughnessIntensity, 1);
    };
    this->loaderMap.Add(MaterialProperties::BSDF, bsdfLoader);

    auto unlitLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.UnlitMaterials.Size());
        MaterialInterface::UnlitMaterial& material = state.UnlitMaterials.Emplace();
        LOAD_TEXTURE(AlbedoMap, CoreGraphics::White1D);
    };
    this->loaderMap.Add(MaterialProperties::Unlit, unlitLoader);

    auto unlit2Loader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.Unlit2Materials.Size());
        MaterialInterface::Unlit2Material& material = state.Unlit2Materials.Emplace();
        LOAD_TEXTURE(Layer1, CoreGraphics::White1D);
        LOAD_TEXTURE(Layer2, CoreGraphics::White1D);
    };
    this->loaderMap.Add(MaterialProperties::Unlit2, unlit2Loader);

    auto unlit3Loader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.Unlit3Materials.Size());
        MaterialInterface::Unlit3Material& material = state.Unlit3Materials.Emplace();
        LOAD_TEXTURE(Layer1, CoreGraphics::White1D);
        LOAD_TEXTURE(Layer2, CoreGraphics::White1D);
        LOAD_TEXTURE(Layer3, CoreGraphics::White1D);
    };
    this->loaderMap.Add(MaterialProperties::Unlit3, unlit3Loader);

    auto unlit4Loader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.Unlit4Materials.Size());
        MaterialInterface::Unlit4Material& material = state.Unlit4Materials.Emplace();
        LOAD_TEXTURE(Layer1, CoreGraphics::White1D);
        LOAD_TEXTURE(Layer2, CoreGraphics::White1D);
        LOAD_TEXTURE(Layer3, CoreGraphics::White1D);
        LOAD_TEXTURE(Layer4, CoreGraphics::White1D);
    };
    this->loaderMap.Add(MaterialProperties::Unlit4, unlit4Loader);

    auto skyboxLoader = [](Ptr<IO::BXmlReader> reader, Materials::MaterialId mat, Util::StringAtom tag) {
        Materials::MaterialSetBufferBinding(mat, state.SkyboxMaterials.Size());
        MaterialInterface::SkyboxMaterial& material = state.SkyboxMaterials.Emplace();
        LOAD_TEXTURE(SkyLayer1, CoreGraphics::White1D);
        LOAD_TEXTURE(SkyLayer2, CoreGraphics::White1D);
        LOAD_FLOAT(Contrast, 1.0f);
        LOAD_FLOAT(Brightness, 1.0f);
    };
    this->loaderMap.Add(MaterialProperties::Skybox, skyboxLoader);

    n_assert_msg(this->loaderMap.Size() == (uint)MaterialProperties::Num, "Missing material loaders, please add a loader for each material in the MaterialProperties enum");

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

        // load surface
        Resources::ResourceName shaderConfig = reader->GetString("template");
        Materials::ShaderConfigServer* server = Materials::ShaderConfigServer::Instance();

        IndexT i = server->shaderConfigsByName.FindIndex(shaderConfig);
        if (i == InvalidIndex)
        {
            n_error("Material '%s', referenced in '%s' could not be found!", shaderConfig.AsString().AsCharPtr(), stream->GetURI().AsString().AsCharPtr());
        }
        ShaderConfig* config = server->shaderConfigsByName.ValueAtIndex(i);

        // Create material, copying the defaults from the material table
        MaterialId id = CreateMaterial({ config });

        // Load using the new material system
        IndexT loaderIndex = this->loaderMap.FindIndex(config->GetMaterialProperties());
        if (loaderIndex != InvalidIndex)
        {
            auto loader = this->loaderMap.ValueAtIndex(loaderIndex);
            if (reader->SetToFirstChild("Params")) do
            {
                loader(reader, id, tag);
            }
            while (reader->SetToNextChild("Params"));
        }

        if (reader->SetToFirstChild("Param")) do
        {
            Util::StringAtom paramName = reader->GetString("name");

            // set variant value which we will use in the surface constants
            IndexT binding = config->GetConstantIndex(paramName);
            IndexT slot = config->GetTextureIndex(paramName);
            if (binding != InvalidIndex)
            {
                MaterialVariant defaultVal = config->GetConstantDefault(binding);
                if (defaultVal.GetType() != MaterialVariant::Type::Invalid)
                {
                    MaterialVariant materialVal = ShaderConfigServer::Instance()->AllocateVariantMemory(defaultVal.type);
                    switch (defaultVal.GetType())
                    {
                        case MaterialVariant::Type::Float:
                            materialVal.Set(reader->GetOptFloat("value", defaultVal.Get<float>()));
                            MaterialSetConstant(id, binding, materialVal);
                            break;
                        case MaterialVariant::Type::Int:
                            materialVal.Set(reader->GetOptInt("value", defaultVal.Get<int32>()));
                            MaterialSetConstant(id, binding, materialVal);
                            break;
                        case MaterialVariant::Type::Bool:
                            materialVal.Set(reader->GetOptBool("value", defaultVal.Get<bool>()));
                            MaterialSetConstant(id, binding, materialVal);
                            break;
                        case MaterialVariant::Type::Vec4:
                            materialVal.Set(reader->GetOptVec4("value", defaultVal.Get<Math::vec4>()));
                            MaterialSetConstant(id, binding, materialVal);
                            break;
                        case MaterialVariant::Type::Vec2:
                            materialVal.Set(reader->GetOptVec2("value", defaultVal.Get<Math::vec2>()));
                            MaterialSetConstant(id, binding, materialVal);
                            break;
                        case MaterialVariant::Type::Mat4:
                            materialVal.Set(reader->GetOptMat4("value", defaultVal.Get<Math::mat4>()));
                            MaterialSetConstant(id, binding, materialVal);
                            break;
                        case MaterialVariant::Type::TextureHandle: // texture handle
                        {
                            Util::String path = reader->GetOptString("value", "");
                            /*if (paramName == "DiffuseMap" || paramName == "AlbedoMap")
                                path = "systex:error.dds";
                                */
                            Resources::ResourceId tex;
                            if (!path.IsEmpty())
                            {
                                tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, tag,
                                    [id, binding, materialVal](Resources::ResourceId rid)
                                    {
                                        CoreGraphics::TextureIdLock _0(rid);
                                        MaterialVariant::TextureHandleTuple tuple{ rid.HashCode64(), CoreGraphics::TextureGetBindlessHandle(rid) };
                                        MaterialVariant tmp = materialVal;
                                        tmp.Set(tuple);
                                        MaterialSetConstant(id, binding, materialVal);
                                        MaterialAddLODTexture(id, rid);
                                    },
                                    [id, binding, materialVal](Resources::ResourceId rid)
                                    {
                                        CoreGraphics::TextureIdLock _0(rid);
                                        MaterialVariant::TextureHandleTuple tuple{ rid.HashCode64(), CoreGraphics::TextureGetBindlessHandle(rid) };
                                        MaterialVariant tmp = materialVal;
                                        tmp.Set(tuple);
                                        MaterialSetConstant(id, binding, materialVal);
                                    });
                                CoreGraphics::TextureIdLock _0(tex);
                                MaterialVariant::TextureHandleTuple tuple{ tex.HashCode64(), CoreGraphics::TextureGetBindlessHandle(tex) };
                                materialVal.Set(tuple);
                            }
                            else
                                tex = Resources::ResourceId(defaultVal.Get<uint64>());

                            CoreGraphics::TextureIdLock _0(tex);
                            MaterialVariant::TextureHandleTuple tuple{ tex.HashCode64(), CoreGraphics::TextureGetBindlessHandle(tex) };
                            materialVal.Set(tuple);
                            MaterialSetConstant(id, binding, materialVal);
                        } break;
                        default: n_error("unhandled enum"); break;
                    }
                }
            }
            else if (slot != InvalidIndex)
            {
                Resources::ResourceId tex = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag, 
                    [id, slot](Resources::ResourceId rid)
                    {
                        CoreGraphics::TextureIdLock _0(rid);
                        MaterialSetTexture(id, slot, rid);
                        MaterialAddLODTexture(id, rid);
                    }, 
                    [id, slot](Resources::ResourceId rid)
                    {
                        CoreGraphics::TextureIdLock _0(rid);
                        MaterialSetTexture(id, slot, rid);
                    });

                CoreGraphics::TextureIdLock _0(tex);
                MaterialSetTexture(id, slot, tex);
            }
            
        } while (reader->SetToNextChild("Param"));

        return id;
    }
    return InvalidMaterialId;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::Unload(const Resources::ResourceId id)
{
    MaterialId material;
    material.resourceId = id.resourceId;
    material.resourceType = id.resourceType;
    DestroyMaterial(material);
}

} // namespace Materials
