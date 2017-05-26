//------------------------------------------------------------------------------
//  modelnodematerial.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "materials/materialtype.h"
#include "materialserver.h"

namespace Materials
{
using namespace Util;

//------------------------------------------------------------------------------
/**
	Private constructor, only the ModelServer may create the central 
	ModelNodeMaterial registry.
*/
MaterialType::MaterialType()
{
	this->nameToCode.Reserve(MaxNumMaterialTypes);
	this->codeToName.Reserve(MaxNumMaterialTypes);
}

//------------------------------------------------------------------------------
/**
*/
MaterialType::Code
MaterialType::FromName(const Name& name)
{
	MaterialType& registry = MaterialServer::Instance()->materialTypeRegistry;
	IndexT index = registry.nameToCode.FindIndex(name);
	if (InvalidIndex != index)
	{
		return registry.nameToCode.ValueAtIndex(index);
	}
	else
	{
		// material hasn't been registered yet
		registry.codeToName.Append(name);
		Code code = registry.codeToName.Size() - 1;
		registry.nameToCode.Add(name, code);
		return code;
	}
}

//------------------------------------------------------------------------------
/**
*/
MaterialType::Name
MaterialType::ToName(Code c)
{
    MaterialType& registry = MaterialServer::Instance()->materialTypeRegistry;
	return registry.codeToName[c];	
}

} // namespace Models
