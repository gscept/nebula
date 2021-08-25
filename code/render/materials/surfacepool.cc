//------------------------------------------------------------------------------
//  surfacepool.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "surfacepool.h"
#include "io/bxmlreader.h"
#include "resources/resourceserver.h"
#include "materialserver.h"

namespace Materials
{

__ImplementClass(Materials::SurfacePool, 'MAPO', Resources::ResourceStreamPool);

//------------------------------------------------------------------------------
/**
*/
void 
SurfacePool::Setup()
{
    this->placeholderResourceName = "sur:system/placeholder.sur";
    this->failResourceName = "sur:system/error.sur";

    // never forget to run this
    ResourceStreamPool::Setup();
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
SurfacePool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
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

        SurfaceId& sid = this->Get<Surface_SurfaceId>(id.resourceId);
        MaterialType*& type = this->Get<Surface_MaterialType>(id.resourceId);

        // get min lod reference
        float& minLod = this->Get<Surface_MinLOD>(id.resourceId);
        minLod = 1.0f;

        // load surface
        Resources::ResourceName materialType = reader->GetString("template");
        Materials::MaterialServer* server = Materials::MaterialServer::Instance();

        if (!server->materialTypesByName.Contains(materialType))
        {
            n_error("Material '%s', referenced in '%s' could not be found!", materialType.AsString().AsCharPtr(), stream->GetURI().AsString().AsCharPtr());
        }
        type = server->materialTypesByName[materialType];

        // add to internal table
        sid = type->CreateSurface();

        if (reader->SetToFirstChild("Param")) do
        {
            Util::StringAtom paramName = reader->GetString("name");

            // set variant value which we will use in the surface constants
            IndexT binding = type->GetSurfaceConstantIndex(sid, paramName);
            IndexT slot = type->GetSurfaceTextureIndex(sid, paramName);
            if (binding != InvalidIndex)
            {
                Util::Variant defaultVal = type->GetSurfaceConstantDefault(sid, binding);
                switch (defaultVal.GetType())
                {
                case Util::Variant::Float:
                    defaultVal.SetFloat(reader->GetOptFloat("value", defaultVal.GetFloat()));
                    type->SetSurfaceConstant(sid, binding, defaultVal);
                    break;
                case Util::Variant::Int:
                    defaultVal.SetInt(reader->GetOptInt("value", defaultVal.GetInt()));
                    type->SetSurfaceConstant(sid, binding, defaultVal);
                    break;
                case Util::Variant::Bool:
                    defaultVal.SetBool(reader->GetOptBool("value", defaultVal.GetBool()));
                    type->SetSurfaceConstant(sid, binding, defaultVal);
                    break;
                case Util::Variant::Vec4:
                    defaultVal.SetVec4(reader->GetOptVec4("value", defaultVal.GetVec4()));
                    type->SetSurfaceConstant(sid, binding, defaultVal);
                    break;
                case Util::Variant::Vec2:
                    defaultVal.SetVec2(reader->GetOptVec2("value", defaultVal.GetVec2()));
                    type->SetSurfaceConstant(sid, binding, defaultVal);
                    break;
                case Util::Variant::Mat4:
                    defaultVal.SetMat4(reader->GetOptMat4("value", defaultVal.GetMat4()));
                    type->SetSurfaceConstant(sid, binding, defaultVal);
                    break;
                case Util::Variant::UInt64: // texture handle
                {
                    const Util::String path = reader->GetOptString("value", "");
                    CoreGraphics::TextureId tex;
                    if (!path.IsEmpty())
                    {
                        tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, tag, 
                            [type, sid, binding, id, &minLod, this](Resources::ResourceId rid)
                            {
                                type->SetSurfaceConstant(sid, binding, CoreGraphics::TextureGetBindlessHandle(rid));
                                this->textureLoadSection.Enter();
                                this->Get<Surface_Textures>(id.resourceId).Append(rid);
                                this->Get<Surface_MinLOD>(id.resourceId) = 1.0f;
                                this->textureLoadSection.Leave();
                            }, 
                            [type, sid, binding](Resources::ResourceId rid)
                            {
                                type->SetSurfaceConstant(sid, binding, CoreGraphics::TextureGetBindlessHandle(rid));
                            });
                        defaultVal = tex.HashCode64();
                    }
                    else
                        tex = CoreGraphics::TextureId(defaultVal.GetUInt64());
                    
                    defaultVal.SetUInt64(tex.HashCode64());
                    type->SetSurfaceConstant(sid, binding, CoreGraphics::TextureGetBindlessHandle(tex));

                    break;
                }
                }
            }
            else if (slot != InvalidIndex)
            {
                CoreGraphics::TextureId tex = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag, 
                    [type, sid, slot, id, &minLod, this](Resources::ResourceId rid)
                    {
                        type->SetSurfaceTexture(sid, slot, rid);
                        this->Get<Surface_Textures>(id.resourceId).Append(rid);
                        this->Get<Surface_MinLOD>(id.resourceId) = 1.0f;
                    }, 
                    [type, sid, slot](Resources::ResourceId rid)
                    {
                        type->SetSurfaceTexture(sid, slot, rid);
                    });
                type->SetSurfaceTexture(sid, slot, tex);
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
SurfacePool::Unload(const Resources::ResourceId id)
{
    const SurfaceId mid = this->Get<Surface_SurfaceId>(id.resourceId);
    MaterialType* type = this->Get<Surface_MaterialType>(id.resourceId);
    type->DestroySurface(mid);

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

//------------------------------------------------------------------------------
/**
*/
void
SurfacePool::SetMaxLOD(const SurfaceResourceId id, const float lod)
{
    this->textureLoadSection.Enter();
    Util::Array<CoreGraphics::TextureId>& textures = this->Get<Surface_Textures>(id.resourceId);
    float& minLod = this->Get<Surface_MinLOD>(id.resourceId);
    if (minLod <= lod)
        return;
    minLod = lod;

    for (IndexT i = 0; i < textures.Size(); i++)
    {
        Resources::SetMaxLOD(textures[i], lod, false);
    }
    this->textureLoadSection.Leave();
}

} // namespace Materials
