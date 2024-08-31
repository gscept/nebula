#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamShaderLoader
    
    Resource loader to setup a Shader object from a stream.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "resources/resourceloader.h"

namespace CoreGraphics
{
void CmdSetShaderProgram(const CoreGraphics::CmdBufferId, const CoreGraphics::ShaderProgramId, bool);
class ShaderLoader : public Resources::ResourceLoader
{
    __DeclareClass(ShaderLoader);

public:

    /// constructor
    ShaderLoader();
    /// destructor
    virtual ~ShaderLoader();


private:
    friend class VkVertexSignatureCache;
    friend class VkPipelineDatabase;
    friend void CoreGraphics::CmdSetShaderProgram(const CoreGraphics::CmdBufferId, const CoreGraphics::ShaderProgramId, bool);

    /// load shader
    ResourceInitOutput InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
    /// reload shader
    Resources::Resource::State ReloadFromStream(const Resources::ResourceId id, const Ptr<IO::Stream>& stream) override;

    /// unload shader
    void Unload(const Resources::ResourceId id) override;


    //__ResourceAllocator(VkShader);

};

}

