#pragma once
//------------------------------------------------------------------------------
/**
    Implements the shader server used by Vulkan.
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/fixedpool.h"
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/config.h"
#include "coregraphics/texture.h"
#include "effectfactory.h"
#include "vkshaderpool.h"
#include "coregraphics/graphicsdevice.h"
#include "shared.h"

namespace Graphics
{
class FrameScript;
}

namespace Vulkan
{

struct BindlessTexturesContext
{
    IndexT texture2DTextureVar;
    SizeT numBoundTextures2D;
    IndexT texture2DMSTextureVar;
    SizeT numBoundTextures2DMS;
    IndexT texture2DArrayTextureVar;
    SizeT numBoundTextures2DArray;
    IndexT texture3DTextureVar;
    SizeT numBoundTextures3D;
    IndexT textureCubeTextureVar;
    SizeT numBoundTexturesCube;
    Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;

};

struct TickParametersContext
{
    IndexT cboSlot;
    IndexT cboOffset;
    CoreGraphics::BufferId cbo;
};

class VkShaderServer : public Base::ShaderServerBase
{
    __DeclareClass(VkShaderServer);
    __DeclareInterfaceSingleton(VkShaderServer);
public:
    /// constructor
    VkShaderServer();
    /// destructor
    virtual ~VkShaderServer();

    /// open the shader server
    bool Open();
    /// close the shader server
    void Close();
    
    /// register new texture
    uint32_t RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, bool depth = false, bool stencil = false);
    /// reregister texture
    void ReregisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, uint32_t slot, bool depth = false, bool stencil = false);
    /// unregister texture
    void UnregisterTexture(const uint32_t id, const CoreGraphics::TextureType type);

    /// get resource table based on frame buffer
    BindlessTexturesContext GetBindlessTextureContext();
    /// get tick parameter context
    TickParametersContext GetTickParametersContext();

    /// set global irradiance and cubemaps
    void SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips);

    /// get tick params constant buffer
    Shared::PerTickParams& GetTickParams();

    /// submit resource changes
    void SubmitTextureDescriptorChanges();
    /// commit texture library to graphics pipeline
    void BindTextureDescriptorSetsGraphics();
    /// commit texture library to compute pipeline
    void BindTextureDescriptorSetsCompute(const CoreGraphics::QueueType queue = CoreGraphics::GraphicsQueueType);

    /// add a pending image view update to the update queue, thread safe
    void AddPendingImageView(CoreGraphics::TextureId tex, VkImageViewCreateInfo viewCreate, uint32_t bind);

    /// setup gbuffer bindings
    void SetupBufferConstants(const Ptr<Frame::FrameScript>& frameScript);

    /// begin frame
    void UpdateResources();
    /// end frame
    void AfterView();

private:

    Util::FixedPool<uint32_t> texture1DPool;
    Util::FixedPool<uint32_t> texture1DArrayPool;
    Util::FixedPool<uint32_t> texture2DPool;
    Util::FixedPool<uint32_t> texture2DMSPool;
    Util::FixedPool<uint32_t> texture2DArrayPool;
    Util::FixedPool<uint32_t> texture3DPool;
    Util::FixedPool<uint32_t> textureCubePool;
    Util::FixedPool<uint32_t> textureCubeArrayPool;

    Util::FixedArray<CoreGraphics::ResourceTableId> resourceTables;
    CoreGraphics::ResourcePipelineId tableLayout;
    IndexT texture2DTextureVar;
    IndexT texture2DMSTextureVar;
    IndexT texture2DArrayTextureVar;
    IndexT texture3DTextureVar;
    IndexT textureCubeTextureVar;

    IndexT normalBufferTextureVar;
    IndexT depthBufferTextureVar;
    IndexT specularBufferTextureVar;
    IndexT depthBufferCopyTextureVar;

    IndexT environmentMapVar;
    IndexT irradianceMapVar;
    IndexT numEnvMipsVar;

    Threading::CriticalSection bindResourceCriticalSection;
    struct _PendingView
    {
        CoreGraphics::TextureId tex;
        VkImageViewCreateInfo createInfo;
        uint32_t bind;
    };

    struct _PendingViewDelete
    {
        VkImageView view;
        uint32_t replaceCounter;
    };

    Threading::SafeQueue<_PendingView> pendingViews;
    Util::Array<_PendingViewDelete> pendingViewDeletes;

    IndexT csmBufferTextureVar;
    IndexT spotlightAtlasShadowBufferTextureVar;

    alignas(16) Shared::PerTickParams tickParams;
    CoreGraphics::BufferId ticksCbo;
    IndexT cboOffset;
    IndexT cboSlot;

    AnyFX::EffectFactory* factory;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkShaderServer::BindTextureDescriptorSetsGraphics()
{
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
    CoreGraphics::SetResourceTable(this->resourceTables[bufferedFrameIndex], NEBULA_TICK_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
VkShaderServer::BindTextureDescriptorSetsCompute(const CoreGraphics::QueueType queue)
{
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
    CoreGraphics::SetResourceTable(this->resourceTables[bufferedFrameIndex], NEBULA_TICK_GROUP, CoreGraphics::ComputePipeline, nullptr, queue);
}

//------------------------------------------------------------------------------
/**
*/
inline Shared::PerTickParams&
VkShaderServer::GetTickParams()
{
    return this->tickParams;
}

} // namespace Vulkan
