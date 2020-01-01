//------------------------------------------------------------------------------
//  surfacepool.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "surfacepool.h"
#include "io/bxmlreader.h"
#include "resources/resourcemanager.h"
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

		this->EnterGet();
		SurfaceRuntime& info = this->Get<0>(id.resourceId);

		// load surface
		Resources::ResourceName materialType = reader->GetString("template");
		Materials::MaterialServer* server = Materials::MaterialServer::Instance();
		MaterialType* type = server->materialTypesByName[materialType];

		// add to internal table
		SurfaceId sid = type->CreateSurface();
		info.id = sid;
		info.type = type;

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
				case Util::Variant::Float4:
					defaultVal.SetFloat4(reader->GetOptFloat4("value", defaultVal.GetFloat4()));
					type->SetSurfaceConstant(sid, binding, defaultVal);
					break;
				case Util::Variant::Float2:
					defaultVal.SetFloat2(reader->GetOptFloat2("value", defaultVal.GetFloat2()));
					type->SetSurfaceConstant(sid, binding, defaultVal);
					break;
				case Util::Variant::Matrix44:
					defaultVal.SetMatrix44(reader->GetOptMatrix44("value", defaultVal.GetMatrix44()));
					type->SetSurfaceConstant(sid, binding, defaultVal);
					break;
				case Util::Variant::UInt64: // texture handle
				{
					const Util::String path = reader->GetOptString("value", "");
					CoreGraphics::TextureId tex;
					if (!path.IsEmpty())
					{
						tex = Resources::CreateResource(path + NEBULA_TEXTURE_EXTENSION, tag, nullptr, nullptr, true);
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
				CoreGraphics::TextureId tex = Resources::CreateResource(reader->GetString("value") + NEBULA_TEXTURE_EXTENSION, tag, nullptr, nullptr, true);
				type->SetSurfaceTexture(sid, slot, tex);
			}
			
		} while (reader->SetToNextChild("Param"));

		this->LeaveGet();
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
	const SurfaceRuntime& runtime = this->Get<0>(id.resourceId);
	const SurfaceId mid = runtime.id;
	MaterialType* type = runtime.type;
	type->DestroySurface(mid);

	this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

} // namespace Materials
