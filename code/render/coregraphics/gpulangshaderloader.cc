//------------------------------------------------------------------------------
//  streamshaderloader.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "coregraphics/gpulangshaderloader.h"
#include "api/loader.h"
#include "util/memory.h"

namespace CoreGraphics
{

__ImplementClass(CoreGraphics::ShaderLoader, 'GPSL', Resources::ResourceLoader);

//------------------------------------------------------------------------------
/**
*/
GPULangShaderLoader::GPULangShaderLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GPULangShaderLoader::~GPULangShaderLoader()
{

}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
GPULangShaderLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(stream->CanBeMapped());

    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    Resources::ResourceLoader::ResourceInitOutput ret;

    GPULang::Loader* loader = GPULang::Alloc<GPULang::Loader>();

    // catch any potential error coming from AnyFX
    if (!loader->Load((const char*)srcData, srcDataSize))
    {
        return ret;
    }

    GPULangShaderCreateInfo shaderInfo;
    shaderInfo.loader = loader;
    shaderInfo.name = job.name;
    ret.id = CreateShader(shaderInfo);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Resources::Resource::State
GPULangShaderLoader::ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream)
{
    void* srcData = stream->Map();
    uint srcDataSize = stream->GetSize();

    // load effect from memory
    AnyFX::ShaderEffect* effect = AnyFX::EffectFactory::Instance()->CreateShaderEffectFromMemory(srcData, srcDataSize);

    // catch any potential error coming from AnyFX
    if (!effect)
    {
        n_error("GPULangShaderLoader::ReloadFromStream(): failed to load shader '%s'!",
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
GPULangShaderLoader::Unload(const Resources::ResourceId res)
{
    ShaderId id = res.resource;
    CoreGraphics::DestroyShader(id);
}

} // namespace CoreGraphics
