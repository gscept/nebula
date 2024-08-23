#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureCache
  
    Resource loader for loading texture data from a Nebula stream. Supports
    synchronous and asynchronous loading.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "gliml.h"

namespace CoreGraphics
{

struct TextureStreamData;

class TextureLoader : public Resources::ResourceLoader
{
    __DeclareClass(TextureLoader);
public:
    /// constructor
    TextureLoader();
    /// destructor
    virtual ~TextureLoader();

private:

    friend void FinishMips(TextureLoader* loader, TextureStreamData* streamData, uint mipBits, const CoreGraphics::TextureId texture, const char* name);

    /// load texture
    Resources::ResourceUnknownId InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// Stream texture
    uint StreamResource(const Resources::ResourceId entry, IndexT frameIndex, uint requestedBits) override;
    /// unload texture
    void Unload(const Resources::ResourceId id) override;
    /// Create load mask based on LOD
    uint LodMask(const Ids::Id32 entry, float lod, bool stream) const override;

    /// Handle before jobs
    void OnBeforeJobs();

    /// Update intermediate loaded state
    void UpdateLoaderSyncState() override;

    Util::FixedArray<Util::Array<CoreGraphics::CmdBufferId>> retiredCommandBuffers;

    // Fill this array with resources that needs updating on the main thread while in a pending state
    Util::Array<Resources::ResourceId> partiallyCompleteResources;
    Util::Array<Resources::ResourceId> finishedResources;
    CoreGraphics::CmdBufferPoolId transferPool, handoverPool;
};

} // namespace CoreGraphics
