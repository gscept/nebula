//------------------------------------------------------------------------------
//  surfacename.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "stdneb.h"
#include "materials/surfacename.h"
#include "materialserver.h"

namespace Materials
{
using namespace Util;

//------------------------------------------------------------------------------
/**
	Private constructor, only the ModelServer may create the central 
	ModelNodeMaterial registry.
*/
SurfaceName::SurfaceName()
{
    this->nameToCode.Reserve(MaxNumSurfaceNames);
    this->codeToName.Reserve(MaxNumSurfaceNames);
}

//------------------------------------------------------------------------------
/**
*/
SurfaceName::Code
SurfaceName::FromName(const Name& name)
{
    SurfaceName& registry = MaterialServer::Instance()->surfaceNameRegistry;
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
SurfaceName::Name
SurfaceName::ToName(Code c)
{
    SurfaceName& registry = MaterialServer::Instance()->surfaceNameRegistry;
	return registry.codeToName[c];	
}

} // namespace Models
