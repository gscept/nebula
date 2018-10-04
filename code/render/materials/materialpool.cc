//------------------------------------------------------------------------------
//  materialpool.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialpool.h"
#include "io/bxmlreader.h"
#include "resources/resourcemanager.h"
#include "materialserver.h"

namespace Materials
{

__ImplementClass(Materials::MaterialPool, 'MAPO', Resources::ResourceStreamPool);

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
MaterialPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream)
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
		MaterialRuntime& info = this->Get<0>(id.allocId);

		// load surface
		Resources::ResourceName materialType = reader->GetString("template");
		Materials::MaterialServer* server = Materials::MaterialServer::Instance();
		MaterialType* type = server->materialTypesByName[materialType];

		// add to internal table
		MaterialInstanceId mid = type->CreateInstance();
		info.id = mid;
		info.type = type;

		if (reader->SetToFirstChild("Param")) do
		{
			Util::Variant defaultVal;
			Util::StringAtom paramName = reader->GetString("name");
			IndexT index;
			if (type->constants.Contains(paramName, index))
			{
				defaultVal = type->constants.ValueAtIndex(index).defaultValue;
			}
			else if (type->textures.Contains(paramName, index))
			{
				defaultVal.SetType(Util::Variant::Int64);
				defaultVal.SetInt64(type->textures.ValueAtIndex(index).defaultValue.HashCode64());
			}
			else
			{
				const Resources::ResourceName& name = this->GetName(id);
				n_warning(Util::String::Sprintf("No parameter matching name '%s' exists in the material template '%s'\n", paramName.AsString().AsCharPtr(), name.Value()).AsCharPtr());
				continue;
			}

			// set variant value which we will use in the surface constants
			switch (defaultVal.GetType())
			{
			case Util::Variant::Float:
				defaultVal.SetFloat(reader->GetOptFloat("value", defaultVal.GetFloat()));
				type->SetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Int:
				defaultVal.SetInt(reader->GetOptInt("value", defaultVal.GetInt()));
				type->SetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Bool:
				defaultVal.SetBool(reader->GetOptBool("value", defaultVal.GetBool()));
				type->SetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Float4:
				defaultVal.SetFloat4(reader->GetOptFloat4("value", defaultVal.GetFloat4()));
				type->SetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Float2:
				defaultVal.SetFloat2(reader->GetOptFloat2("value", defaultVal.GetFloat2()));
				type->SetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::Matrix44:
				defaultVal.SetMatrix44(reader->GetOptMatrix44("value", defaultVal.GetMatrix44()));
				type->SetConstant(mid, paramName, defaultVal);
				break;
			case Util::Variant::String: // texture
			{
				CoreGraphics::TextureId tex = Resources::CreateResource(reader->GetString("value"), tag, nullptr, nullptr);
				type->SetTexture(mid, paramName, tex);
				defaultVal.SetUInt64(tex.HashCode64());
				break;
			}
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
MaterialPool::Unload(const Resources::ResourceId id)
{
	const MaterialRuntime& runtime = this->Get<0>(id.allocId);
	const MaterialInstanceId mid = runtime.id;
	MaterialType* type = runtime.type;
	type->DestroyInstance(mid);
}

} // namespace Materials
