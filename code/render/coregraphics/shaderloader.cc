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
Resources::ResourceLoader::ResourceInitOutput
ShaderLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());

    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    Resources::ResourceLoader::ResourceInitOutput ret;

    // load effect from memory
    AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

    // catch any potential error coming from AnyFX
    if (!effect)
    {
        return ret;
    }

    ShaderCreateInfo shaderInfo;
    shaderInfo.effect = effect;
    shaderInfo.name = job.name;
    ret.id = CreateShader(shaderInfo);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Resources::Resource::State
ShaderLoader::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
{
    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    // load effect from memory
    AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

    // catch any potential error coming from AnyFX
    if (!effect)
    {
        n_error("VkStreamShaderLoader::ReloadFromStream(): failed to load shader '%s'!",
            this->GetName(id.resourceId).Value());
        return Resources::Resource::Failed;
    }

    ShaderId shader = id.resource;
    ReloadShader(shader, effect);
    return Resources::Resource::Loaded;
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
