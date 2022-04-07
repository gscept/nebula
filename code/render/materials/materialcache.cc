//------------------------------------------------------------------------------
//  surfacecache.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialcache.h"
#include "io/bxmlreader.h"
#include "resources/resourceserver.h"
#include "shaderconfigserver.h"

namespace Materials
{

__ImplementClass(Materials::MaterialCache, 'MAPO', Resources::ResourceStreamCache);

//------------------------------------------------------------------------------
/**
*/
void 
MaterialCache::Setup()
{
    this->placeholderResourceName = "sur:system/placeholder.sur";
    this->failResourceName = "sur:system/error.sur";

    // never forget to run this
    ResourceStreamCache::Setup();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceCache::LoadStatus
MaterialCache::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    Ptr<IO::BXmlReader> reader = IO::BXmlReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        // make sure it's a valid frame shader file
        if (!reader->HasNode("/Nebula/Surface"))
        {
            n_error("StreamSurfaceMaterialLoader: '%s' is not a valid surface!", stream->GetURI().AsString().AsCharPtr());
            return Failed;
        }

        // send to first node
        reader->SetToNode("/Nebula/Surface");

        MaterialId& sid = this->Get<Surface_MaterialId>(id.resourceId);
        ShaderConfig*& type = this->Get<Surface_MaterialType>(id.resourceId);

        // get min lod reference
        float& minLod = this->Get<Surface_MinLOD>(id.resourceId);
        minLod = 1.0f;

        // load surface
        Resources::ResourceName shaderConfig = reader->GetString("template");
        Materials::ShaderConfigServer* server = Materials::ShaderConfigServer::Instance();

        if (!server->shaderConfigsByName.Contains(shaderConfig))
        {
            n_error("Material '%s', referenced in '%s' could not be found!", shaderConfig.AsString().AsCharPtr(), stream->GetURI().AsString().AsCharPtr());
        }
        type = server->shaderConfigsByName[shaderConfig];

        // Create material, copying the defaults from the material table
        sid = type->CreateMaterial();

        if (reader->SetToFirstChild("Param")) do
        {
            Util::StringAtom paramName = reader->GetString("name");

            // set variant value which we will use in the surface constants
            IndexT binding = type->GetMaterialConstantIndex(paramName);
            IndexT slot = type->GetMaterialTextureIndex(paramName);
            if (binding != InvalidIndex)
            {
                ShaderConfigVariant defaultVal = type->GetMaterialConstantDefault(sid, binding);
                if (defaultVal.GetType() != ShaderConfigVariant::Type::Invalid)
                {
                    ShaderConfigVariant materialVal = ShaderConfigServer::Instance()->AllocateVariantMemory(defaultVal.type);
                    switch (defaultVal.GetType())
                    {
                        case ShaderConfigVariant::Type::Float:
                            materialVal.Set(reader->GetOptFloat("value", defaultVal.Get<float>()));
                            type->SetMaterialConstant(sid, binding, materialVal);
                            break;
                        case ShaderConfigVariant::Type::Int:
                            materialVal.Set(reader->GetOptInt("value", defaultVal.Get<int32>()));
                            type->SetMaterialConstant(sid, binding, materialVal);
                            break;
                        case ShaderConfigVariant::Type::Bool:
                            materialVal.Set(reader->GetOptBool("value", defaultVal.Get<bool>()));
                            type->SetMaterialConstant(sid, binding, materialVal);
                            break;
                        case ShaderConfigVariant::Type::Vec4:
                            materialVal.Set(reader->GetOptVec4("value", defaultVal.Get<Math::vec4>()));
                            type->SetMaterialConstant(sid, binding, materialVal);
                            break;
                        case ShaderConfigVariant::Type::Vec2:
                            materialVal.Set(reader->GetOptVec2("value", defaultVal.Get<Math::vec2>()));
                            type->SetMaterialConstant(sid, binding, materialVal);
                            break;
                        case ShaderConfigVariant::Type::Mat4:
                            materialVal.Set(reader->GetOptMat4("value", defaultVal.Get<Math::mat4>()));
                            type->SetMaterialConstant(sid, binding, materialVal);
                            break;
                        case ShaderConfigVariant::Type::TextureHandle: // texture handle
                        {
                            const Util::String path = reader->GetOptString("value", "");
                            CoreGraphics::TextureId tex;
                            if (!path.IsEmpty())
                            {
                                tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, tag,
                                    [type, sid, binding, id, &minLod, materialVal, this](Resources::ResourceId rid)
                                {
                                    ShaderConfigVariant::TextureHandleTuple tuple{ rid.HashCode64(), CoreGraphics::TextureGetBindlessHandle(rid) };
                                    ShaderConfigVariant tmp = materialVal;
                                    tmp.Set(tuple);
                                    type->SetMaterialConstant(sid, binding, materialVal);
                                    this->textureLoadSection.Enter();
                                    this->Get<Surface_Textures>(id.resourceId).Append(rid);
                                    this->Get<Surface_MinLOD>(id.resourceId) = 1.0f;
                                    this->textureLoadSection.Leave();
                                },
                                    [type, sid, binding, materialVal](Resources::ResourceId rid)
                                {
                                    ShaderConfigVariant::TextureHandleTuple tuple{ rid.HashCode64(), CoreGraphics::TextureGetBindlessHandle(rid) };
                                    ShaderConfigVariant tmp = materialVal;
                                    tmp.Set(tuple);
                                    type->SetMaterialConstant(sid, binding, materialVal);
                                });
                                ShaderConfigVariant::TextureHandleTuple tuple{ tex.HashCode64(), CoreGraphics::TextureGetBindlessHandle(tex) };
                                materialVal.Set(tuple);
                            }
                            else
                                tex = CoreGraphics::TextureId(defaultVal.Get<uint64>());

                            ShaderConfigVariant::TextureHandleTuple tuple{ tex.HashCode64(), CoreGraphics::TextureGetBindlessHandle(tex) };
                            materialVal.Set(tuple);
                            type->SetMaterialConstant(sid, binding, materialVal);

                            break;
                        }
                    }
                }
            }
            else if (slot != InvalidIndex)
            {
                CoreGraphics::TextureId tex = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag, 
                    [type, sid, slot, id, &minLod, this](Resources::ResourceId rid)
                    {
                        type->SetMaterialTexture(sid, slot, rid);
                        this->Get<Surface_Textures>(id.resourceId).Append(rid);
                        this->Get<Surface_MinLOD>(id.resourceId) = 1.0f;
                    }, 
                    [type, sid, slot](Resources::ResourceId rid)
                    {
                        type->SetMaterialTexture(sid, slot, rid);
                    });
                type->SetMaterialTexture(sid, slot, tex);
            }
            
        } while (reader->SetToNextChild("Param"));

        return Success;
    }
    return Failed;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialCache::Unload(const Resources::ResourceId id)
{
    const MaterialId mid = this->Get<Surface_MaterialId>(id.resourceId);
    ShaderConfig* type = this->Get<Surface_MaterialType>(id.resourceId);
    type->DestroyMaterial(mid);

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialCache::SetMaxLOD(const MaterialResourceId id, const float lod)
{
    Threading::CriticalScope scope(&this->textureLoadSection);
    Util::Array<CoreGraphics::TextureId>& textures = this->Get<Surface_Textures>(id.resourceId);
    float& minLod = this->Get<Surface_MinLOD>(id.resourceId);
    if (minLod <= lod)
        return;
    minLod = lod;

    for (IndexT i = 0; i < textures.Size(); i++)
    {
        Resources::SetMaxLOD(textures[i], lod, false);
    }
}

} // namespace Materials
