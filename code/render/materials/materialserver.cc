//------------------------------------------------------------------------------
//  materialserver.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "materialserver.h"
#include "resources/resourcemanager.h"
#include "materialpool.h"
#include "materialtype.h"
#include "io/ioserver.h"
#include "io/bxmlreader.h"
#include "coregraphics/batchgroup.h"
#include "coregraphics/shaderserver.h"

namespace Materials
{
__ImplementClass(Materials::MaterialServer, 'MASV', Core::RefCounted)
__ImplementSingleton(Materials::MaterialServer)
//------------------------------------------------------------------------------
/**
*/
MaterialServer::MaterialServer() :
	isOpen(false),
	currentType(nullptr)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
MaterialServer::~MaterialServer()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialServer::Open()
{
	n_assert(!this->isOpen);
	this->isOpen = true;

	// load base materials first
	this->LoadMaterialTypes("mat:base.xml");

	// okay, now load the rest of the materials
	Util::Array<Util::String> materialTables = IO::IoServer::Instance()->ListFiles("mat:", "*.xml");
	for (IndexT i = 0; i < materialTables.Size(); i++)
	{
		if (materialTables[i] == "base.xml") continue;
		this->LoadMaterialTypes("mat:" + materialTables[i]);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialServer::Close()
{
	this->materialAllocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
bool
MaterialServer::LoadMaterialTypes(const IO::URI& file)
{
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
	Ptr<IO::BXmlReader> reader = IO::BXmlReader::Create();
	reader->SetStream(stream);

	if (reader->Open())
	{
		// check to see it's a valid Nebula3 materials file
		if (!reader->HasNode("/Nebula3/Materials"))
		{
			n_error("MaterialLoader: '%s' is not a valid material XML!", file.AsString().AsCharPtr());
			return false;
		}
		reader->SetToNode("/Nebula3/Materials");

		// parse materials
		if (reader->SetToFirstChild("Material")) do
		{
			MaterialType* type = this->materialAllocator.Alloc<MaterialType>();
			this->materialTypes.Append(type);

			type->name = reader->GetString("name");
			type->description = reader->GetOptString("desc", "");
			type->group = reader->GetOptString("group", "Ungrouped");
			type->id = this->materialTypes.Size() - 1; // they are load-time, so we can safetly use this as the id
			materialTypesByName.Add(type->name, type);

			n_assert2(!type->name.ContainsCharFromSet("|"), "Name of material may not contain character '|' since it's used to denote multiple inheritance");

			bool isVirtual = reader->GetOptBool("virtual", false);
			if (isVirtual)
			{
				if (reader->HasAttr("vertexType"))
				{
					n_error("Material '%s' is virtual and is not allowed to have a type defined", type->name.AsCharPtr());
				}
			}
			else
			{
				Util::String vtype = reader->GetString("vertexType");
				type->vertexType = vtype.HashCode();
			}
			type->isVirtual = isVirtual;

			Util::String inherits = reader->GetOptString("inherits", "");

			// load inherited material
			if (!inherits.IsEmpty())
			{
				Util::Array<Util::String> inheritances = inherits.Tokenize("|");
				IndexT i;
				for (i = 0; i < inheritances.Size(); i++)
				{
					Util::String otherMat = inheritances[i];
					IndexT index = this->materialTypesByName.FindIndex(otherMat);
					if (index == InvalidIndex)
						n_error("Material '%s' is not defined or loaded yet.", otherMat.AsCharPtr());
					else
					{
						MaterialType* mat = this->materialTypesByName.ValueAtIndex(index);
						type->batches.AppendArray(mat->batches);
						type->programs.Merge(mat->programs);
						type->textures.Merge(mat->textures);
						type->constants.Merge(mat->constants);
					}
				}
			}

			// parse passes
			if (reader->SetToFirstChild("Pass")) do
			{
				// get batch name
				Util::String batchName = reader->GetString("batch");
				Util::String shaderFeatures = reader->GetString("variation");

				// convert batch name to model node type
				CoreGraphics::BatchGroup::Code code = CoreGraphics::BatchGroup::FromName(batchName);

				//get shader
				Util::String shaderName = reader->GetString("shader");
				Resources::ResourceName shaderResId = Resources::ResourceName("shd:" + shaderName + ".fxb");
				CoreGraphics::ShaderId shd = CoreGraphics::ShaderServer::Instance()->GetShader(shaderResId);
				CoreGraphics::ShaderFeature::Mask mask = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(shaderFeatures);
				CoreGraphics::ShaderProgramId program = CoreGraphics::ShaderGetProgram(shd, mask);

				if (program == CoreGraphics::ShaderProgramId::Invalid())
					n_warning("WARNING: Material '%s' failed to load program with features '%s'\n", type->name.AsCharPtr(), shaderFeatures.AsCharPtr());

				n_assert(!type->programs.Contains(code));
				type->batches.Append(code);
				type->programs.Add(code, program);

				// add material to server
				Util::Array<Materials::MaterialType*>& mats = this->materialTypesByBatch.AddUnique(code);
				mats.Append(type);

			} while (reader->SetToNextChild("Pass"));

			// parse parameters
			if (reader->SetToFirstChild("Param")) do
			{
				// parse parameters
				Util::String name = reader->GetString("name");
				Util::String ptype = reader->GetString("type");
				Util::String desc = reader->GetOptString("desc", "");
				Util::String editType = reader->GetOptString("edit", "raw");
				bool system = reader->GetOptBool("system", false);

				if (ptype.BeginsWithString("texture"))
				{
					MaterialTexture texture;
					if (ptype == "texture1d") texture.type = CoreGraphics::Texture1D;
					else if (ptype == "texture2d") texture.type = CoreGraphics::Texture2D;
					else if (ptype == "texture3d") texture.type = CoreGraphics::Texture3D;
					else if (ptype == "texturecube") texture.type = CoreGraphics::TextureCube;
					else if (ptype == "texture1darray") texture.type = CoreGraphics::Texture1DArray;
					else if (ptype == "texture2darray") texture.type = CoreGraphics::Texture2DArray;
					else if (ptype == "texture3darray") texture.type = CoreGraphics::Texture3DArray;
					else if (ptype == "texturecubearray") texture.type = CoreGraphics::TextureCubeArray;
					else
					{
						n_error("Invalid texture type %s\n", ptype.AsCharPtr());
					}
					texture.defaultValue = Resources::CreateResource(reader->GetString("defaultValue") + NEBULAT_TEXTURE_EXTENSION, "material types", nullptr, nullptr, true);
					texture.system = system;
					type->textures.Add(name, texture);
				}
				else
				{
					MaterialConstant constant;
					constant.type = Util::Variant::StringToType(ptype);
					constant.system = system;
					switch (constant.type)
					{
					case Util::Variant::Float:
						constant.defaultValue.SetFloat(reader->GetOptFloat("defaultValue", 0.0f));
						constant.min.SetFloat(reader->GetOptFloat("min", 0.0f));
						constant.max.SetFloat(reader->GetOptFloat("max", 1.0f));
						break;
					case Util::Variant::Int:
						constant.defaultValue.SetInt(reader->GetOptInt("defaultValue", 0));
						constant.min.SetInt(reader->GetOptInt("min", 0));
						constant.max.SetInt(reader->GetOptInt("max", 1));
						break;
					case Util::Variant::Bool:
						constant.defaultValue.SetBool(reader->GetOptBool("defaultValue", false));
						constant.min.SetBool(false);
						constant.max.SetBool(true);
						break;
					case Util::Variant::Float4:
						constant.defaultValue.SetFloat4(reader->GetOptFloat4("defaultValue", Math::float4(0, 0, 0, 0)));
						constant.min.SetFloat4(reader->GetOptFloat4("min", Math::float4(0, 0, 0, 0)));
						constant.max.SetFloat4(reader->GetOptFloat4("max", Math::float4(1, 1, 1, 1)));
						break;
					case Util::Variant::Float2:
						constant.defaultValue.SetFloat2(reader->GetOptFloat2("defaultValue", Math::float2(0, 0)));
						constant.min.SetFloat2(reader->GetOptFloat2("min", Math::float2(0, 0)));
						constant.max.SetFloat2(reader->GetOptFloat2("max", Math::float2(1, 1)));
						break;
					case Util::Variant::Matrix44:
						constant.defaultValue.SetMatrix44(reader->GetOptMatrix44("defaultValue", Math::matrix44::identity()));
						break;
					default:
						n_error("Unknown material parameter type %s\n", ptype.AsCharPtr());
					}

					type->constants.Add(name, constant);
				}
			} while (reader->SetToNextChild("Param"));

			// go throught constants and collect groups
			Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Set<IndexT>> uniqueGroups;

		} while (reader->SetToNextChild("Material"));

		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
MaterialType* 
MaterialServer::GetMaterial(const Resources::ResourceName& type)
{
	MaterialType* mat = this->materialTypesByName[type];
	return mat;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<MaterialType*>*
MaterialServer::GetMaterialTypesByBatch(CoreGraphics::BatchGroup::Code code)
{
	IndexT i = this->materialTypesByBatch.FindIndex(code);
	if (i == InvalidIndex)  return nullptr;
	else					return &this->materialTypesByBatch.ValueAtIndex(code, i);
}

} // namespace Base
