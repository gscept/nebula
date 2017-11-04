//------------------------------------------------------------------------------
//  materialloader.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "materials/material.h"
#include "materials/materialloader.h"
#include "materials/materialinstance.h"
#include "materials/materialserver.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/shaderserver.h"
#include "resources/resourceid.h"
#include "io/ioserver.h"
#include "util/string.h"
#include "resources/resourcemanager.h"
#include "math/float4.h"

namespace Materials
{

using namespace Math;
using namespace Util;
using namespace IO;
using namespace Resources;
using namespace CoreGraphics;

Util::Stack<IO::URI> MaterialLoader::LoadingStack;
//------------------------------------------------------------------------------
/**
*/
Ptr<MaterialPalette>
MaterialLoader::LoadMaterialPalette(const Resources::ResourceName& name, const IO::URI& uri, bool optional)
{
	Ptr<MaterialPalette> palette;
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
	Ptr<BXmlReader> xmlReader = BXmlReader::Create();
	xmlReader->SetStream(stream);
	if (xmlReader->Open())
	{
		// check to see it's a valid Nebula3 materials file
		if (!xmlReader->HasNode("/Nebula3/Materials"))
		{
			n_error("MaterialLoader: '%s' is not a valid material palette!", uri.AsString().AsCharPtr());
			return palette;
		}

		MaterialLoader::LoadingStack.Push(uri);
		xmlReader->SetToNode("/Nebula3");
		palette = MaterialPalette::Create();
		palette->SetName(name);
		ParsePalette(xmlReader, palette);
		xmlReader->Close();
		MaterialLoader::LoadingStack.Pop();
	}
	else if (!optional)
	{
		n_error("MaterialLoader: failed to load material palette '%s'!", uri.AsString().AsCharPtr());
	}
    else
    {
        n_printf("MaterialLoader: failed to load material palette '%s'!", uri.AsString().AsCharPtr());
    }
	return palette;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::ParsePalette(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<MaterialPalette>& materialPalette)
{
	// parse dependencies
	if (xmlReader->SetToFirstChild("Dependency")) do
	{
		ParseDependency(xmlReader, materialPalette);
	}
	while (xmlReader->SetToNextChild("Dependency"));
	xmlReader->SetToParent();
	xmlReader->SetToNode("Materials");

	// parse materials
	if (xmlReader->SetToFirstChild("Material")) do 
	{
		ParseMaterial(xmlReader, materialPalette);
	} 
	while (xmlReader->SetToNextChild("Material"));
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::ParseMaterial(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<MaterialPalette>& materialPalette)
{
	// create the new material
	MaterialServer* matServer = MaterialServer::Instance();
	Ptr<Material> material = Material::Create();
	String name = xmlReader->GetString("name");
	String desc = xmlReader->GetOptString("desc", "");
    String inherits = xmlReader->GetOptString("inherits", "");
	String group = xmlReader->GetOptString("group", "Ungrouped");

	n_assert2(!name.ContainsCharFromSet("|"), "Name of material may not contain character '|' since it's used to denote multiple inheritance");
	
	bool isVirtual = xmlReader->GetOptBool("virtual", false);
	if (isVirtual)
	{
		if (xmlReader->HasAttr("type"))
		{
			n_error("Material '%s' is virtual and is not allowed to have a type defined", name.AsCharPtr());
		}	
	}
	else
	{
		String type = xmlReader->GetString("type");
		material->SetFeatures(matServer->FeatureStringToMask(type));
	}

	// setup material
	material->SetName(name);
	material->SetVirtual(isVirtual);
	material->SetDescription(desc);	
	material->SetGroup(group);
	material->SetCode(Materials::MaterialType::FromName(name));

    // load inherited material
    if (!inherits.IsEmpty())
    {
		Array<String> inheritances = inherits.Tokenize("|");
		IndexT i;
		for (i = 0; i < inheritances.Size(); i++)
		{
			String otherMat = inheritances[i];
			if (!matServer->HasMaterial(otherMat)) n_error("Material '%s' is not defined or loaded yet.", otherMat.AsCharPtr());
			const Ptr<Material>& mat = matServer->GetMaterialByName(otherMat);
			material->LoadInherited(mat);
		}
    }

	// parse passes
	if (xmlReader->SetToFirstChild("Pass")) do 
	{
		ParseMaterialPass(xmlReader, material);
	} 
	while (xmlReader->SetToNextChild("Pass"));

	// parse parameters
	if (xmlReader->SetToFirstChild("Param")) do 
	{
		ParseParameter(xmlReader, material);
	} 
	while (xmlReader->SetToNextChild("Param"));

	// add material to palette
	materialPalette->AddMaterial(material);	
	matServer->AddMaterial(material);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::ParseMaterialPass(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<Material>& material)
{
	n_assert(0 != material);

	// get batch name
	String batchName = xmlReader->GetString("batch");
	String shaderFeatures = xmlReader->GetString("variation");

	// convert batch name to model node type
    Graphics::BatchGroup::Code code = Graphics::BatchGroup::FromName(batchName);

	//get shader
	String shaderName = xmlReader->GetString("shader");
	ResourceId shaderResId = ResourceId("shd:" + shaderName);
	const Ptr<Shader>& shader= ShaderServer::Instance()->GetShader(shaderResId);

	// add feature mask
	material->AddPass(code, shader, ShaderServer::Instance()->FeatureStringToMask(shaderFeatures));
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::ParseDependency(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<MaterialPalette>& material)
{
	MaterialServer* matServer = MaterialServer::Instance();
	String list = xmlReader->GetString("list");

	if (LoadingStack.Contains(list))
	{
		n_error("Trying to load cyclic material list %s from %s", list.AsCharPtr(), material->GetName().Value());
	}
	else
	{
		if (!matServer->LookupMaterialPalette(list))
		{
			n_error("Could not load dependency material list %s", list.AsCharPtr());
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialLoader::ParseParameter(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<Material>& material)
{
	Material::MaterialParameter param;

	// parse parameters
	String name = xmlReader->GetString("name");
	String type = xmlReader->GetString("type");
    String desc = xmlReader->GetOptString("desc", "");
	String editType = xmlReader->GetOptString("edit", "raw");
	bool system = xmlReader->GetOptBool("system", false);
	Variant var;
	Variant min;
	Variant max;

	Variant::Type varType = Variant::StringToType(type);
	switch (varType)
	{
	case Variant::Float:
		var.SetFloat(xmlReader->GetOptFloat("defaultValue", 0.0f));
		min.SetFloat(xmlReader->GetOptFloat("min", 0.0f));
		max.SetFloat(xmlReader->GetOptFloat("max", 1.0f));
		break;
	case Variant::Int:
		var.SetInt(xmlReader->GetOptInt("defaultValue", 0));
		min.SetInt(xmlReader->GetOptInt("min", 0));
		max.SetInt(xmlReader->GetOptInt("max", 1));
		break;
	case Variant::Bool:
		var.SetBool(xmlReader->GetOptBool("defaultValue", false));
		min.SetBool(false);
		max.SetBool(true);
		break;
	case Variant::Float4:
		var.SetFloat4(xmlReader->GetOptFloat4("defaultValue", float4(0,0,0,0)));
		min.SetFloat4(xmlReader->GetOptFloat4("min", float4(0,0,0,0)));
		max.SetFloat4(xmlReader->GetOptFloat4("max", float4(1,1,1,1)));
		break;
	case Variant::Float2:
		var.SetFloat2(xmlReader->GetOptFloat2("defaultValue", float2(0,0)));
		min.SetFloat2(xmlReader->GetOptFloat2("min", float2(0,0)));
		max.SetFloat2(xmlReader->GetOptFloat2("max", float2(1,1)));
		break;
	case Variant::Matrix44:
		var.SetMatrix44(xmlReader->GetOptMatrix44("defaultValue", matrix44::identity()));
		break;
	case Variant::String:
        var.SetString(xmlReader->GetOptString("defaultValue", "tex:system/placeholder.dds"));
        break;
	}		

	// set values
	param.name = name;
    param.desc = desc;
	param.defaultVal = var;
	param.min = min;
	param.max = max;
	param.system = system;
	param.editType = Material::MaterialParameter::EditTypeFromString(editType);

	// create key-value pair
	KeyValuePair<StringAtom, Material::MaterialParameter> kvp(name, param);

	// add to material
    material->AddParam(name, param);
}
} // namespace Materials