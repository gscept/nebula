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
    ResourceLoader::ResourceInitOutput InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
    /// Stream texture
    ResourceLoader::ResourceStreamOutput StreamResource(const ResourceLoadJob& job) override;
    /// unload texture
    void Unload(const Resources::ResourceId id) override;
    /// Create load mask based on LOD
    uint LodMask(const _StreamData& stream, float lod, bool async) const override;

    /// Update intermediate loaded state
    void UpdateLoaderSyncState() override;


    // First step of the load chain is to invoke a mip load on the main thread
    struct MipLoadMainThread
    {
        Resources::ResourceId id;
        uint bits;
        Util::Array<Memory::RangeAllocation> rangesToFlush;
        CoreGraphics::CmdBufferId transferCmdBuf, graphicsCmdBuf;
    };
    Threading::SafeQueue<MipLoadMainThread> mipLoadsToSubmit;

    // Then, we have to wait for the GPU to finish before we can issue a mip finish on the loader thread
    struct MipHandoverLoaderThread
    {
        uint64_t handoverSubmissionId;
        uint bits;
        Util::Array<Memory::RangeAllocation> rangesToFree;
        CoreGraphics::CmdBufferId uploadBuffer, receiveBuffer;
    };
    Util::HashTable<Resources::ResourceId, Util::Array<MipHandoverLoaderThread>> mipHandovers;
    Threading::CriticalSection handoverLock;

    CoreGraphics::CmdBufferPoolId asyncTransferPool, immediateTransferPool, asyncHandoverPool, immediateHandoverPool;
};

} // namespace CoreGraphics
