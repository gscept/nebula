//------------------------------------------------------------------------------
// vkshaderserver.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaderserver.h"
#include "effectfactory.h"
#include "coregraphics/shaderpool.h"
#include "vkgraphicsdevice.h"
#include "vkshaderpool.h"
#include "vkshader.h"

#include "shared.h"

using namespace Resources;
using namespace CoreGraphics;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShaderServer, 'VKSS', Base::ShaderServerBase);
__ImplementInterfaceSingleton(Vulkan::VkShaderServer);
//------------------------------------------------------------------------------ 
/**
*/
VkShaderServer::VkShaderServer()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VkShaderServer::~VkShaderServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
VkShaderServer::Open()
{
    n_assert(!this->IsOpen());

    // create anyfx factory
    this->factory = n_new(AnyFX::EffectFactory);
    ShaderServerBase::Open();

    auto func = [](uint32_t& val, IndexT i) -> void {
        val = i;
    };

    this->pendingViews.SetSignalOnEnqueueEnabled(false);

    this->texturePool.SetSetupFunc(func);
    this->texturePool.Resize(Shared::MAX_TEXTURES);

    // create shader state for textures, and fetch variables
    ShaderId shader = VkShaderServer::Instance()->GetShader("shd:shared.fxb"_atm);

    this->textureTextureVar = ShaderGetResourceSlot(shader, "Textures2D");
    this->tableLayout = ShaderGetResourcePipeline(shader);

    this->ticksCbo = CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer);
    this->cboSlot = ShaderGetResourceSlot(shader, "PerTickParams");

    this->resourceTables.Resize(CoreGraphics::GetNumBufferedFrames());
    IndexT i;
    for (i = 0; i < this->resourceTables.Size(); i++)
    {
        this->resourceTables[i] = ShaderCreateResourceTable(shader, NEBULA_TICK_GROUP, this->resourceTables.Size());
        CoreGraphics::ObjectSetName(this->resourceTables[i], "Main Tick Group Descriptor");
    }

    this->normalBufferTextureVar = ShaderGetConstantBinding(shader, "NormalBuffer");
    this->depthBufferTextureVar = ShaderGetConstantBinding(shader, "DepthBuffer");
    this->specularBufferTextureVar = ShaderGetConstantBinding(shader, "SpecularBuffer");
    this->depthBufferCopyTextureVar = ShaderGetConstantBinding(shader, "DepthBufferCopy");

    this->environmentMapVar = ShaderGetConstantBinding(shader, "EnvironmentMap");
    this->irradianceMapVar = ShaderGetConstantBinding(shader, "IrradianceMap");
    this->numEnvMipsVar = ShaderGetConstantBinding(shader, "NumEnvMips");
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::Close()
{
    n_assert(this->IsOpen());
    n_delete(this->factory);
    IndexT i;
    for (i = 0; i < this->resourceTables.Size(); i++)
    {
        DestroyResourceTable(this->resourceTables[i]);
    }
    ShaderServerBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
VkShaderServer::RegisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, bool depth, bool stencil)
{
    uint32_t idx = this->texturePool.Alloc();
    IndexT var = this->textureTextureVar;

    ResourceTableTexture info;
    info.tex = tex;
    info.index = idx;
    info.sampler = InvalidSamplerId;
    info.isDepth = depth;
    info.isStencil = stencil;
    info.slot = this->textureTextureVar;

    // update textures for all tables
    this->bindResourceCriticalSection.Enter();
    IndexT i;
    for (i = 0; i < this->resourceTables.Size(); i++)
    {
        ResourceTableSetTexture(this->resourceTables[i], info);
    }
    this->bindResourceCriticalSection.Leave();

    return idx;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::ReregisterTexture(const CoreGraphics::TextureId& tex, CoreGraphics::TextureType type, uint32_t slot, bool depth, bool stencil)
{
    ResourceTableTexture info;
    info.tex = tex;
    info.index = slot;
    info.sampler = InvalidSamplerId;
    info.isDepth = depth;
    info.isStencil = stencil;
    info.slot = this->textureTextureVar;

    // update textures for all tables
    this->bindResourceCriticalSection.Enter();
    IndexT i;
    for (i = 0; i < this->resourceTables.Size(); i++)
    {
        ResourceTableSetTexture(this->resourceTables[i], info);
    }
    this->bindResourceCriticalSection.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::UnregisterTexture(const uint32_t id, const CoreGraphics::TextureType type)
{
    this->texturePool.Free(id);
}

//------------------------------------------------------------------------------
/**
*/
TickParametersContext VkShaderServer::GetTickParametersContext()
{
    TickParametersContext ret;
    ret.cboSlot = this->cboSlot;
    ret.cboOffset = this->cboOffset;
    ret.cbo = this->ticksCbo;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::SetGlobalEnvironmentTextures(const CoreGraphics::TextureId& env, const CoreGraphics::TextureId& irr, const SizeT numMips)
{
    this->tickParams.EnvironmentMap = CoreGraphics::TextureGetBindlessHandle(env);
    this->tickParams.IrradianceMap = CoreGraphics::TextureGetBindlessHandle(irr);
    this->tickParams.NumEnvMips = numMips;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::SetupBufferConstants()
{   
    this->tickParams.NormalBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("NormalBuffer"));
    this->tickParams.SpecularBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("SpecularBuffer"));
    this->tickParams.DepthBuffer = TextureGetBindlessHandle(CoreGraphics::GetTexture("ZBuffer"));
    this->tickParams.DepthBufferCopy = TextureGetBindlessHandle(CoreGraphics::GetTexture("ZBufferCopy"));
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::UpdateResources()
{
    // just allocate the memory
    this->cboOffset = CoreGraphics::AllocateGraphicsConstantBufferMemory(MainThreadConstantBuffer, sizeof(Shared::PerTickParams));
    IndexT bufferedFrameIndex = GetBufferedFrameIndex();

    VkDevice dev = GetCurrentDevice();

    // setup new views for newly streamed LODs
    Util::Array<_PendingView> pendingViewsThisFrame(32, 32);
    pendingViews.DequeueAll(pendingViewsThisFrame);

    for (int i = 0; i < pendingViewsThisFrame.Size(); i++)
    {
        const _PendingView& pend = pendingViewsThisFrame[i];

        textureAllocator.EnterGet();
        VkTextureRuntimeInfo& info = textureAllocator.Get<Texture_RuntimeInfo>(pend.tex.resourceId);
        VkImageView oldView = info.view;
        VkResult res = vkCreateImageView(GetCurrentDevice(), &pend.createInfo, nullptr, &info.view);
        n_assert(res == VK_SUCCESS);
        textureAllocator.LeaveGet();

        _PendingViewDelete pendingDelete;
        pendingDelete.view = oldView;
        pendingDelete.replaceCounter = 0;
        pendingViewDeletes.Append(pendingDelete);

        // update texture entries for all tables
        this->ReregisterTexture(pend.tex, info.type, info.bind);
    }

    // delete views which have been discarded due to LOD streaming
    for (int i = this->pendingViewDeletes.Size() - 1; i >= 0; i--)
    {
        // if we have cycled through all our frames, safetly delete the view
        if (this->pendingViewDeletes[i].replaceCounter == CoreGraphics::GetNumBufferedFrames())
        {
            //vkDestroyImageView(dev, this->pendingViewDeletes[i].view, nullptr);
            this->pendingViewDeletes.EraseIndex(i);
            continue;
        }
        this->pendingViewDeletes[i].replaceCounter++;
    }

    // update resource table for this frame
    ResourceTableSetConstantBuffer(this->resourceTables[bufferedFrameIndex], { this->ticksCbo, this->cboSlot, 0, false, false, sizeof(Shared::PerTickParams), (SizeT)this->cboOffset });
    ResourceTableCommitChanges(this->resourceTables[bufferedFrameIndex]);

    // set global tick group resources
    CoreGraphics::SetTickResourceTable(this->resourceTables[bufferedFrameIndex]);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::AfterView()
{
    // update the constant buffer with the data accumulated in this frame
    CoreGraphics::SetGraphicsConstants(MainThreadConstantBuffer, this->cboOffset, this->tickParams);
}

//------------------------------------------------------------------------------
/**
*/
void
VkShaderServer::AddPendingImageView(CoreGraphics::TextureId tex, VkImageViewCreateInfo viewCreate, uint32_t bind)
{
    _PendingView pend;
    pend.tex = tex;
    pend.createInfo = viewCreate;
    pend.bind = bind;
    this->pendingViews.Enqueue(pend);
}

} // namespace Vulkan
