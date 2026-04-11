//------------------------------------------------------------------------------
//  streamshaderloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/shaderloader.h"

namespace CoreGraphics
{

__ImplementClass(CoreGraphics::ShaderLoader, 'VKSL', Resources::ResourceLoader);

//------------------------------------------------------------------------------
/**
*/

ShaderLoader::ShaderLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/

ShaderLoader::~ShaderLoader()
{

}

//------------------------------------------------------------------------------
/**
*/
void
ShaderLoader::Unload(const Resources::ResourceId res)
{
    ShaderId id = res.resource;
    CoreGraphics::DestroyShader(id);
}

} // namespace CoreGraphics
