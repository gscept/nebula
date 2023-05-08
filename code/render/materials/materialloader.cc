//------------------------------------------------------------------------------
//  materialloader.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialloader.h"
#include "io/bxmlreader.h"
#include "resources/resourceserver.h"
#include "shaderconfigserver.h"

namespace Materials
{

__ImplementClass(Materials::MaterialLoader, 'MALO', Resources::ResourceLoader);

//------------------------------------------------------------------------------
/**
*/
void 
MaterialLoader::Setup()
{
    this->placeholderResourceName = "sur:system/placeholder.sur";
    this->failResourceName = "sur:system/error.sur";

    // never forget to run this
    ResourceLoader::Setup();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceUnknownId
MaterialLoader::LoadFromStream(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    Ptr<IO::BXmlReader> reader = IO::BXmlReader::Create();
    reader->SetStream(stream);
    if (reader->Open())
    {
        // make sure it's a valid frame shader file
        if (!reader->HasNode("/Nebula/Surface"))
        {
            n_error("MaterialLoader: '%s' is not a valid surface!", stream->GetURI().AsString().AsCharPtr());
            return Failed;
        }

        // send to first node
        reader->SetToNode("/Nebula/Surface");

        // get min lod reference
        float minLod = 1.0f;

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
                                path = "tex:system/error.dds";
                                */
                            Resources::ResourceId tex;
                            if (!path.IsEmpty())
                            {
                                tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, tag,
                                    [config, id, binding, &minLod, materialVal, this](Resources::ResourceId rid)
                                    {
                                        MaterialVariant::TextureHandleTuple tuple{ rid.HashCode64(), CoreGraphics::TextureGetBindlessHandle(rid) };
                                        MaterialVariant tmp = materialVal;
                                        tmp.Set(tuple);
                                        MaterialSetConstant(id, binding, materialVal);
                                        MaterialAddLODTexture(id, rid);
                                    },
                                    [config, id, binding, materialVal](Resources::ResourceId rid)
                                    {
                                        MaterialVariant::TextureHandleTuple tuple{ rid.HashCode64(), CoreGraphics::TextureGetBindlessHandle(rid) };
                                        MaterialVariant tmp = materialVal;
                                        tmp.Set(tuple);
                                        MaterialSetConstant(id, binding, materialVal);
                                    });
                                MaterialVariant::TextureHandleTuple tuple{ tex.HashCode64(), CoreGraphics::TextureGetBindlessHandle(tex) };
                                materialVal.Set(tuple);
                            }
                            else
                                tex = Resources::ResourceId(defaultVal.Get<uint64>());

                            MaterialVariant::TextureHandleTuple tuple{ tex.HashCode64(), CoreGraphics::TextureGetBindlessHandle(tex) };
                            materialVal.Set(tuple);
                            MaterialSetConstant(id, binding, materialVal);

                            break;
                        }
                    }
                }
            }
            else if (slot != InvalidIndex)
            {
                Resources::ResourceId tex = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag, 
                    [config, id, slot, &minLod, this](Resources::ResourceId rid)
                    {
                        MaterialSetTexture(id, slot, rid);
                        MaterialAddLODTexture(id, rid);
                    }, 
                    [config, id, slot](Resources::ResourceId rid)
                    {
                        MaterialSetTexture(id, slot, rid);
                    });

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
