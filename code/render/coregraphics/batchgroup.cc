//------------------------------------------------------------------------------
//  batchgroup.cc
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/batchgroup.h"
#include "graphics/graphicsserver.h"

namespace CoreGraphics
{

using namespace Util;

//------------------------------------------------------------------------------
/**
    Private constructor, only the ModelServer may create the central 
    ModelNodeType registry.
*/
BatchGroup::BatchGroup()
{
    this->nameToCode.Reserve(NumBatchGroups);
    this->codeToName.Reserve(NumBatchGroups);
}

//------------------------------------------------------------------------------
/**
*/
BatchGroup::Code
BatchGroup::FromName(const Name& name)
{
    BatchGroup& registry = Graphics::GraphicsServer::Instance()->batchGroupRegistry;
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
BatchGroup::Name
BatchGroup::ToName(Code c)
{
    BatchGroup& registry = Graphics::GraphicsServer::Instance()->batchGroupRegistry;
    return registry.codeToName[c];
}

} // namespace Graphics
