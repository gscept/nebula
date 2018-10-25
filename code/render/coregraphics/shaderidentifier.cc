//------------------------------------------------------------------------------
//  shaderidentifier.cc
//  (C) 2015-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/shaderidentifier.h"
#include "shaderserver.h"

namespace CoreGraphics
{

using namespace Util;

//------------------------------------------------------------------------------
/**
    Private constructor, only the ShaderServer may create the central 
    ShaderIdentifier registry.
*/
ShaderIdentifier::ShaderIdentifier()
{
    this->nameToCode.Reserve(MaxNumShaderIdentifiers);
    this->codeToName.Reserve(MaxNumShaderIdentifiers);
}

//------------------------------------------------------------------------------
/**
*/
ShaderIdentifier::Code
ShaderIdentifier::FromName(const Name& name)
{
    ShaderIdentifier& registry = ShaderServer::Instance()->shaderIdentifierRegistry;
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
ShaderIdentifier::Name
ShaderIdentifier::ToName(Code c)
{
    ShaderIdentifier& registry = ShaderServer::Instance()->shaderIdentifierRegistry;
    return registry.codeToName[c];
}

} // namespace Models
