//------------------------------------------------------------------------------
//  streamshaderloader.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "coregraphics/gpulangshaderloader.h"
#include "api/loader.h"
#include "util/memory.h"

namespace CoreGraphics
{

__ImplementClass(CoreGraphics::GPULangShaderLoader, 'GPSL', Resources::ResourceLoader);

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

    void* srcData = stream->MemoryMap();
    uint srcDataSize = stream->GetSize();

    Resources::ResourceLoader::ResourceInitOutput ret;

    GPULang::Loader* loader = new GPULang::Loader;

    if (!loader->Load((const char*)srcData, srcDataSize))
    {
        ret.id = CoreGraphics::InvalidShaderId;
        return ret;
    }

    GPULangShaderCreateInfo shaderInfo;
    shaderInfo.loader = loader;
    shaderInfo.name = job.name;
    ret.id = CreateShader(shaderInfo);
    stream->Close();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
GPULangShaderLoader::ReinitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    void* srcData = stream->MemoryMap();
    uint srcDataSize = stream->GetSize();

    Resources::ResourceLoader::ResourceInitOutput ret;
    ret.id = job.id.resource;

    GPULang::Loader* loader = new GPULang::Loader;

    if (!loader->Load((const char*)srcData, srcDataSize))
    {
        ret.id = CoreGraphics::InvalidShaderId;
    }

    GPULangShaderCreateInfo shaderInfo;
    shaderInfo.loader = loader;
    shaderInfo.name = this->GetName(job.id);
    ShaderId shader = job.id.resource;
    if (!CoreGraphics::ReloadShader(shader.id, shaderInfo))
    {
        n_printf("Failed to reload shader %s, keeping the old one\n", stream->GetURI().LocalPath().AsCharPtr());
    }
    stream->Close();
    return ret;
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
