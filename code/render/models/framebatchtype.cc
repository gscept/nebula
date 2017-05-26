//------------------------------------------------------------------------------
//  modelnodetype.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/framebatchtype.h"
#include "models/modelserver.h"

namespace Models
{

using namespace Util;

//------------------------------------------------------------------------------
/**
    Private constructor, only the ModelServer may create the central 
    ModelNodeType registry.
*/
BatchType::BatchType()
{
    this->nameToCode.Reserve(MaxNumModelNodeTypes);
    this->codeToName.Reserve(MaxNumModelNodeTypes);
}

//------------------------------------------------------------------------------
/**
*/
BatchType::Code
BatchType::FromName(const Name& name)
{
    BatchType& registry = ModelServer::Instance()->modelNodeTypeRegistry;
    IndexT index = registry.nameToCode.FindIndex(name);
    if (InvalidIndex != index)
    {
        return registry.nameToCode.ValueAtIndex(index);
    }
    else
    {
        // name hasn't been registered yet
        registry.codeToName.Append(name);
        Code code = registry.codeToName.Size() - 1;
        registry.nameToCode.Add(name, code);
        return code;
    }
}

//------------------------------------------------------------------------------
/**
*/
BatchType::Name
BatchType::ToName(Code c)
{
    BatchType& registry = ModelServer::Instance()->modelNodeTypeRegistry;
    return registry.codeToName[c];
}

} // namespace Models
