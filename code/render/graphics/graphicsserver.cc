//------------------------------------------------------------------------------
// graphicsserver.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "graphicsserver.h"
#include "graphicscontext.h"
#include "view.h"
#include "stage.h"
#include "resources/resourceserver.h"
#include "coregraphics/vertexsignaturecache.h"
#include "coregraphics/streamtexturecache.h"
#include "coregraphics/memorytexturecache.h"
#include "coregraphics/memorymeshcache.h"
#include "coregraphics/shaderpool.h"
#include "coregraphics/streammeshcache.h"
#include "coreanimation/streamanimationcache.h"
#include "characters/streamskeletoncache.h"
#include "models/streammodelcache.h"
#include "renderutil/drawfullscreenquad.h"

namespace Graphics
{

__ImplementClass(Graphics::GraphicsServer, 'GFXS', Core::RefCounted);
__ImplementSingleton(Graphics::GraphicsServer);

Jobs::JobPortId GraphicsServer::renderSystemsJobPort;
//------------------------------------------------------------------------------
/**
*/
GraphicsServer::GraphicsServer() :
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
GraphicsServer::~GraphicsServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::Open()
{
    n_assert(!this->isOpen);
    this->timer = FrameSync::FrameSyncTimer::HasInstance() ? FrameSync::FrameSyncTimer::Instance() : FrameSync::FrameSyncTimer::Create();
    this->isOpen = true;

    this->displayDevice = CoreGraphics::DisplayDevice::Create();
    this->displayDevice->Open();

    CoreGraphics::GraphicsDeviceCreateInfo gfxInfo { 
        { 1_MB, 30_MB },    // Graphics - main threads get 1 MB of constant memory, visibility thread (objects) gets 50
        { 1_MB, 0_MB },     // Compute - main threads get 1 MB of constant memory, visibility thread (objects) gets 0
        {
            128_MB,         // device local memory block size, textures and vertex/index buffers
            32_MB,          // memory to use for temporary host buffers which are copied to the GPU
            32_MB,          // memory used to read back from the GPU
            128_MB,         // manually flushed memory block size, constant buffers, storage buffers
        },
        3,                  // number of simultaneous frames (3 = triple buffering, 2 = ... you get the idea)
        false               // validation
    };
    this->graphicsDevice = CoreGraphics::CreateGraphicsDevice(gfxInfo);

    Jobs::CreateJobPortInfo info =
    {
        "RenderJobPort",
        4,
        System::Cpu::Core1 | System::Cpu::Core2 | System::Cpu::Core3 | System::Cpu::Core4,
        UINT_MAX
    };
    renderSystemsJobPort = CreateJobPort(info);
    if (this->graphicsDevice)
    {

        // register graphics context pools
        Resources::ResourceServer::Instance()->RegisterMemoryPool(CoreGraphics::VertexSignatureCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterMemoryPool(CoreGraphics::MemoryTextureCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterMemoryPool(CoreGraphics::MemoryMeshCache::RTTI);

        Resources::ResourceServer::Instance()->RegisterStreamPool("dds", CoreGraphics::StreamTextureCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("fxb", CoreGraphics::ShaderCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("nax3", CoreAnimation::StreamAnimationCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("nsk3", Characters::StreamSkeletonCache::RTTI); 
        Resources::ResourceServer::Instance()->RegisterStreamPool("nvx2", CoreGraphics::StreamMeshCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("sur", Materials::MaterialCache::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("n3", Models::StreamModelCache::RTTI);

        // setup internal pool pointers for convenient access (note, will also assert if texture, shader, model or mesh pools is not registered yet!)
        CoreGraphics::layoutPool = Resources::GetMemoryPool<CoreGraphics::VertexSignatureCache>();
        CoreGraphics::texturePool = Resources::GetMemoryPool<CoreGraphics::MemoryTextureCache>();
        CoreGraphics::meshPool = Resources::GetMemoryPool<CoreGraphics::MemoryMeshCache>();

        CoreGraphics::shaderPool = Resources::GetStreamPool<CoreGraphics::ShaderCache>();
        Models::modelPool = Resources::GetStreamPool<Models::StreamModelCache>();
        Materials::surfacePool = Resources::GetStreamPool<Materials::MaterialCache>();

        CoreAnimation::animPool = Resources::GetStreamPool<CoreAnimation::StreamAnimationCache>();
        Characters::skeletonPool = Resources::GetStreamPool<Characters::StreamSkeletonCache>();

        RenderUtil::DrawFullScreenQuad::Setup();

        // load base textures before setting up major subsystems
        const unsigned char white = 0xFF;
        const unsigned char black = 0x00;
        CoreGraphics::TextureCreateInfo texInfo;
        texInfo.usage = CoreGraphics::TextureUsage::SampleTexture;

        texInfo.tag = "system";
        texInfo.format = CoreGraphics::PixelFormat::R8;
        texInfo.bindless = false;

        texInfo.type = CoreGraphics::TextureType::Texture1D;
        texInfo.name = "White1D";
        texInfo.buffer = &white;
        CoreGraphics::White1D = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::Texture1DArray;
        texInfo.name = "White1DArray";
        texInfo.buffer = &white;
        CoreGraphics::White1DArray = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::Texture2D;
        texInfo.name = "White2D";
        texInfo.buffer = &white;
        CoreGraphics::White2D = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::Texture2D;
        texInfo.name = "Black2D";
        texInfo.buffer = &black;
        CoreGraphics::Black2D = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::Texture2DArray;
        texInfo.name = "White2DArray";
        texInfo.buffer = &white;
        CoreGraphics::White2DArray = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::Texture3D;
        texInfo.name = "White3D";
        texInfo.buffer = &white;
        CoreGraphics::White3D = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::TextureCube;
        texInfo.name = "WhiteCube";
        texInfo.buffer = &white;
        CoreGraphics::WhiteCube = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::TextureCubeArray;
        texInfo.name = "WhiteCubeArray";
        texInfo.buffer = &white;
        CoreGraphics::WhiteCubeArray = CoreGraphics::CreateTexture(texInfo);

        const unsigned int red = 0x000000FF;
        const unsigned int green = 0x0000FF00;
        const unsigned int blue = 0x00FF0000;
        texInfo.type = CoreGraphics::TextureType::Texture2D;
        texInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;

        texInfo.name = "Red2D";
        texInfo.buffer = &red;
        CoreGraphics::Red2D = CoreGraphics::CreateTexture(texInfo);

        texInfo.name = "Green2D";
        texInfo.buffer = &green;
        CoreGraphics::Green2D = CoreGraphics::CreateTexture(texInfo);

        texInfo.name = "Blue2D";
        texInfo.buffer = &blue;
        CoreGraphics::Blue2D = CoreGraphics::CreateTexture(texInfo);

        this->shaderServer = CoreGraphics::ShaderServer::Create();
        this->shaderServer->Open();

        this->frameServer = Frame::FrameServer::Create();
        this->frameServer->Open();

        this->materialServer = Materials::ShaderConfigServer::Create();
        this->materialServer->Open();

        this->transformDevice = CoreGraphics::TransformDevice::Create();
        this->transformDevice->Open();

        this->shapeRenderer = CoreGraphics::ShapeRenderer::Create();
        this->shapeRenderer->Open();
        
        this->textRenderer = CoreGraphics::TextRenderer::Create();
        this->textRenderer->Open();

        // start timer
        this->timer->StartTime();

        // tell the resource manager to load default resources once we are done setting everything up
        Resources::ResourceServer::Instance()->LoadDefaultResources();
    }
    else
    {
        n_error("Failed to create render device");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::Close()
{
    n_assert(this->isOpen);
    
    this->isOpen = false;

    RenderUtil::DrawFullScreenQuad::Discard();

    this->textRenderer->Close();
    this->textRenderer = nullptr;

    this->shapeRenderer->Close();
    this->shapeRenderer = nullptr;

    this->transformDevice->Close();
    this->transformDevice = nullptr;

    this->materialServer->Close();
    this->materialServer = nullptr;

    this->frameServer->Close();
    this->frameServer = nullptr;

    this->shaderServer->Close();
    this->shaderServer = nullptr;

    this->displayDevice->Close();
    this->displayDevice = nullptr;

    this->timer->StopTime();
    this->timer = nullptr;

    if (this->graphicsDevice) 
        CoreGraphics::DestroyGraphicsDevice();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::RegisterGraphicsContext(GraphicsContextFunctionBundle* context, GraphicsContextState* state)
{
    this->contexts.Append(context);
    this->states.Append(state);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::UnregisterGraphicsContext(GraphicsContextFunctionBundle* context)
{
    IndexT i = this->contexts.FindIndex(context);
    n_assert(i != InvalidIndex);
    this->contexts.EraseIndex(i);
    this->states.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::OnWindowResized(CoreGraphics::WindowId wndId)
{
    CoreGraphics::DisplayMode const mode = CoreGraphics::WindowGetDisplayMode(wndId);
    for (IndexT i = 0; i < this->contexts.Size(); ++i)
    {
        if (this->contexts[i]->OnWindowResized != nullptr)
        {
            this->contexts[i]->OnWindowResized(wndId.id24, mode.GetWidth(), mode.GetHeight());
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
GraphicsEntityId
GraphicsServer::CreateGraphicsEntity()
{
    GraphicsEntityId id;
    this->entityPool.Allocate(id.id);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::DiscardGraphicsEntity(const GraphicsEntityId id)
{
    this->entityPool.Deallocate(id.id);
}

//------------------------------------------------------------------------------
/**
*/
bool
GraphicsServer::IsValidGraphicsEntity(const GraphicsEntityId id)
{
    return this->entityPool.IsValid(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Graphics::Stage>
GraphicsServer::CreateStage(const Util::StringAtom& name, bool main)
{
    Ptr<Stage> stage = Stage::Create();
    this->stages.Append(stage);
    return stage;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::DiscardStage(const Ptr<Stage>& stage)
{
    IndexT i = this->stages.FindIndex(stage);
    n_assert(i != InvalidIndex);
    this->stages.EraseIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::BeginFrame()
{
    N_SCOPE(BeginFrame, Graphics);
    this->timer->UpdateTimePolling();

    this->frameContext.frameIndex = this->timer->GetFrameIndex();
    this->frameContext.frameTime = this->timer->GetFrameTime();
    this->frameContext.time = this->timer->GetTime();
    this->frameContext.ticks = this->timer->GetTicks();
    this->frameContext.bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // update shader server
    this->shaderServer->BeforeFrame();

    // Collect garbage
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        auto state = this->states[i];
        state->CleanupDelayedRemoveQueue();

        // give contexts a chance to defragment their data
        if (state->Defragment != nullptr)
            state->Defragment();
    }

    N_MARKER_BEGIN(ContextBegin, Graphics);
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->StageBits)
            *this->contexts[i]->StageBits = Graphics::OnBeginStage;
        if (this->contexts[i]->OnBegin != nullptr)
            this->contexts[i]->OnBegin(this->frameContext);
    }
    N_MARKER_END();

    // go through views and call prepare view
    N_MARKER_BEGIN(ContextPrepareView, Graphics);
    for (i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        IndexT j;
        for (j = 0; j < this->contexts.Size(); j++)
        {
            if (this->contexts[j]->StageBits)
                *this->contexts[j]->StageBits = Graphics::OnPrepareViewStage;
            if (this->contexts[j]->OnPrepareView != nullptr)
                this->contexts[j]->OnPrepareView(view, this->frameContext);
        }       
    }
    N_MARKER_END();

    // begin frame
    CoreGraphics::BeginFrame(this->frameContext.frameIndex);

    // update frame context after begin frame
    this->frameContext.bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    N_MARKER_BEGIN(ContextBeforeFrame, Graphics);
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->StageBits)
            *this->contexts[i]->StageBits = Graphics::OnBeforeFrameStage;
        if (this->contexts[i]->OnBeforeFrame != nullptr)
            this->contexts[i]->OnBeforeFrame(this->frameContext);
    }
    N_MARKER_END();

    // consider this whole block of code viable for updating resource tables
    CoreGraphics::ResourceTableBlock(false);

    N_MARKER_BEGIN(ContextUpdateResources, Graphics);
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->StageBits)
            *this->contexts[i]->StageBits = Graphics::OnUpdateResourcesStage;
        if (this->contexts[i]->OnUpdateResources != nullptr)
            this->contexts[i]->OnUpdateResources(this->frameContext);
    }
    N_MARKER_END();

    // update shader server resources (textures and tick params)
    this->shaderServer->UpdateResources();

    // go through views and call prepare view
    N_MARKER_BEGIN(ContextUpdateViewResources, Graphics);
    for (i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        // update view resources (camera)
        view->UpdateResources(this->frameContext.frameIndex, this->frameContext.bufferIndex);

        IndexT j;
        for (j = 0; j < this->contexts.Size(); j++)
        {
            if (this->contexts[j]->StageBits)
                *this->contexts[j]->StageBits = Graphics::OnUpdateViewResourcesStage;
            if (this->contexts[j]->OnUpdateViewResources != nullptr)
                this->contexts[j]->OnUpdateViewResources(view, this->frameContext);
        }
    }
    N_MARKER_END();

    this->currentView = nullptr;

    // finish resource updates
    CoreGraphics::ResourceTableBlock(true);
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::BeforeViews()
{
    N_SCOPE(BeforeViews, Graphics);

    // wait for visibility
    N_MARKER_BEGIN(ContextWaitForWork, Graphics);
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->StageBits)
            *this->contexts[i]->StageBits = Graphics::OnWaitForWorkStage;
        if (this->contexts[i]->OnWaitForWork != nullptr)
            this->contexts[i]->OnWaitForWork(this->frameContext);
    }
    N_MARKER_END();

    N_MARKER_BEGIN(ContextWorkFinished, Graphics);
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->StageBits)
            *this->contexts[i]->StageBits = Graphics::OnWorkFinishedStage;
        if (this->contexts[i]->OnWorkFinished != nullptr)
            this->contexts[i]->OnWorkFinished(this->frameContext);
    }
    N_MARKER_END();

    // go through views and call before view
    for (i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];
        
        if (!view->enabled)
            continue;

        // begin frame on view, this will construct view build jobs
        this->currentView = view;
        this->currentView->BeginFrame(this->frameContext.frameIndex, this->frameContext.time, this->frameContext.bufferIndex);

        N_MARKER_BEGIN(ContextBeforeView, Graphics);
        IndexT j;
        for (j = 0; j < this->contexts.Size(); j++)
        {
            if (this->contexts[i]->StageBits)
                *this->contexts[i]->StageBits = Graphics::OnBeforeViewStage;
            if (this->contexts[j]->OnBeforeView != nullptr)
                this->contexts[j]->OnBeforeView(view, this->frameContext);
        }
        N_MARKER_END();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::RenderViews()
{
    N_SCOPE(RenderViews, Graphics);
    IndexT i;

    // No more constant updates from this point
    CoreGraphics::LockConstantUpdates();

    // go through views and call before view
    for (i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        if (!view->enabled)
            continue;

        view->Render(this->frameContext.frameIndex, this->frameContext.time, this->frameContext.bufferIndex);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::EndViews()
{
    N_SCOPE(EndViews, Graphics);

    // go through views and call before view
    IndexT i;
    for (i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        if (!view->enabled)
            continue;

        this->shaderServer->AfterView();
        this->currentView->EndFrame(this->frameContext.frameIndex, this->frameContext.time, this->frameContext.bufferIndex);
        
        N_MARKER_BEGIN(ContextAfterView, Graphics);
        IndexT j;
        for (j = 0; j < this->contexts.Size(); j++)
        {
            if (this->contexts[i]->StageBits)
                *this->contexts[i]->StageBits = Graphics::OnAfterViewStage;
            if (this->contexts[j]->OnAfterView != nullptr)
                this->contexts[j]->OnAfterView(view, this->frameContext);
        }
        N_MARKER_END();
    }

    this->currentView = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::EndFrame()
{
    N_SCOPE(EndFrame, Graphics);

    // stop the graphics side frame
    CoreGraphics::EndFrame(this->frameContext.frameIndex);

    // finish frame and prepare for the next one
    N_MARKER_BEGIN(ContextAfterFrame, Graphics);
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->StageBits) 
            *this->contexts[i]->StageBits = Graphics::OnAfterFrameStage;
        if (this->contexts[i]->OnAfterFrame != nullptr)
            this->contexts[i]->OnAfterFrame(this->frameContext);
    }
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::NewFrame()
{
    // Wait and get new frame
    CoreGraphics::NewFrame();

    // Open up for constant updates after waiting
    CoreGraphics::UnlockConstantUpdates();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Graphics::View>
GraphicsServer::CreateView(const Util::StringAtom& name, const IO::URI& framescript)
{
    Ptr<View> view = View::Create();
    Ptr<Frame::FrameScript> frameScript = Frame::FrameServer::Instance()->LoadFrameScript(name.AsString() + "_framescript", framescript);
    frameScript->Build();

    view->script = frameScript;
    this->views.Append(view);

    // invoke all interested contexts
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->OnViewCreated != nullptr)
            this->contexts[i]->OnViewCreated(view);
    }
    return view;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Graphics::View> 
GraphicsServer::CreateView(const Util::StringAtom& name)
{
    Ptr<View> view = View::Create();
    view->script = nullptr;
    this->views.Append(view);

    // invoke all interested contexts
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->OnViewCreated != nullptr)
            this->contexts[i]->OnViewCreated(view);
    }
    return view;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::DiscardView(const Ptr<View>& view)
{
    IndexT idx = this->views.FindIndex(view);
    n_assert(idx != InvalidIndex);
    this->views.EraseIndex(idx);
    // invoke all interested contexts    
    for (IndexT i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->OnDiscardView != nullptr)
            this->contexts[i]->OnDiscardView(view);
    }
    view->script->Discard();
}


//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::SetCurrentView(const Ptr<View>& view)
{
    this->currentView = view;
}


//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::RenderDebug(uint32_t flags)
{    
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->OnRenderDebug != nullptr)
            this->contexts[i]->OnRenderDebug(flags);
    }
}

} // namespace Graphics
