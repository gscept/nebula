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
#include "coregraphics/textureloader.h"
#include "coregraphics/shaderloader.h"
#include "coregraphics/meshloader.h"
#include "coreanimation/animationloader.h"
#include "characters/skeletonloader.h"
#include "models/modelloader.h"
#include "renderutil/drawfullscreenquad.h"

#include "bindlessregistry.h"
#include "globalconstants.h"

namespace Graphics
{

__ImplementClass(Graphics::GraphicsServer, 'GFXS', Core::RefCounted);
__ImplementSingleton(Graphics::GraphicsServer);

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
        32_MB,                      // Constant buffer memory
        256_MB,                      // Vertex memory size
        64_MB,                       // Index memory size
        256_MB,
        {
            128_MB,                 // Device local memory block size
            32_MB,                  // Host coherent memory block size
            128_MB,                 // Host cached memory block size
            8_MB,                   // Device <-> host mirrored memory block size
        },
        0x10000, 0x100000, 0x100,   // Number of queries
        3,                          // Number of simultaneous frames (3 = triple buffering, 2 = ... you get the idea)
        true                       // Validation
    };
    this->graphicsDevice = CoreGraphics::CreateGraphicsDevice(gfxInfo);

    if (this->graphicsDevice)
    {

        // Register graphics resource loaders
        Resources::ResourceServer::Instance()->RegisterStreamPool("dds", CoreGraphics::TextureLoader::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("fxb", CoreGraphics::ShaderLoader::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("nax3", CoreAnimation::AnimationLoader::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("nsk3", Characters::SkeletonLoader::RTTI); 
        Resources::ResourceServer::Instance()->RegisterStreamPool("nvx2", CoreGraphics::MeshLoader::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("sur", Materials::MaterialLoader::RTTI);
        Resources::ResourceServer::Instance()->RegisterStreamPool("n3", Models::ModelLoader::RTTI);

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
        texInfo.layers = 6;
        texInfo.buffer = &white;
        CoreGraphics::WhiteCube = CoreGraphics::CreateTexture(texInfo);

        texInfo.type = CoreGraphics::TextureType::TextureCubeArray;
        texInfo.name = "WhiteCubeArray";
        texInfo.layers = 6;
        texInfo.buffer = &white;
        CoreGraphics::WhiteCubeArray = CoreGraphics::CreateTexture(texInfo);

        const unsigned int red = 0x000000FF;
        const unsigned int green = 0x0000FF00;
        const unsigned int blue = 0x00FF0000;
        texInfo.type = CoreGraphics::TextureType::Texture2D;
        texInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;

        texInfo.name = "Red2D";
        texInfo.buffer = &red;
        texInfo.layers = 1;
        CoreGraphics::Red2D = CoreGraphics::CreateTexture(texInfo);

        texInfo.name = "Green2D";
        texInfo.buffer = &green;
        CoreGraphics::Green2D = CoreGraphics::CreateTexture(texInfo);

        texInfo.name = "Blue2D";
        texInfo.buffer = &blue;
        CoreGraphics::Blue2D = CoreGraphics::CreateTexture(texInfo);

        // Setup shader server
        this->shaderServer = CoreGraphics::ShaderServer::Create();
        this->shaderServer->Open();

        // Setup bindless registry and global constants hub
        BindlessRegistryCreateInfo bindlessRegistryInfo;
        CreateBindlessRegistry(bindlessRegistryInfo);

        GlobalConstantsCreateInfo globalConstantsInfo;
        CreateGlobalConstants(globalConstantsInfo);

        this->frameServer = Frame::FrameServer::Create();
        this->frameServer->Open();

        this->materialServer = Materials::ShaderConfigServer::Create();
        this->materialServer->Open();

        this->shapeRenderer = CoreGraphics::ShapeRenderer::Create();
        this->shapeRenderer->Open();
        
        this->textRenderer = CoreGraphics::TextRenderer::Create();
        this->textRenderer->Open();

        // start timer
        if (!this->timer->IsTimeRunning())
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

    // Make sure to flush the graphics commands before shutting down
    RenderUtil::DrawFullScreenQuad::Discard();

    this->textRenderer->Close();
    this->textRenderer = nullptr;

    this->shapeRenderer->Close();
    this->shapeRenderer = nullptr;

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
Ptr<Graphics::View>
GraphicsServer::CreateView(const Util::StringAtom& name, const IO::URI& framescript)
{
    Ptr<View> view = View::Create();
    Ptr<Frame::FrameScript> frameScript = Frame::FrameServer::Instance()->LoadFrameScript(name.AsString() + "_framescript", framescript);

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

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::RunPreLogic()
{
    N_SCOPE(PreLogic, Graphics);
    this->timer->UpdateTimePolling();

    this->frameContext.frameIndex = this->timer->GetFrameIndex();
    this->frameContext.frameTime = this->timer->GetFrameTime();
    this->frameContext.time = this->timer->GetTime();
    this->frameContext.ticks = this->timer->GetTicks();
    this->frameContext.bufferIndex = CoreGraphics::GetBufferedFrameIndex();

    // Update pending texture views
    this->shaderServer->UpdateResources();

    // update shader server
    Graphics::AllocateGlobalConstants();
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

    // Consider this whole block of code viable for updating resource tables
    CoreGraphics::ResourceTableBlock(false);

    N_MARKER_BEGIN(ContextPreLogic, Graphics);
    for (auto& call : this->preLogicCalls)
    {
        call(this->frameContext);
    }
    N_MARKER_END();

    // Go through views and call before view
    for (IndexT i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        if (!view->enabled)
            continue;

        // begin frame on view, this will construct view build jobs
        this->currentView = view;
        view->UpdateConstants();
        N_MARKER_BEGIN(ContextPerView, Graphics);
        for (auto& call : this->preLogicViewCalls)
        {
            call(view, this->frameContext);
        }
        N_MARKER_END();
        this->currentView = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/

void GraphicsServer::RunPostLogic()
{
    N_SCOPE(PostLogic, Graphics);

    N_MARKER_BEGIN(ContextPostLogic, Graphics);
    for (auto& call : this->postLogicCalls)
    {
        call(this->frameContext);
    }
    N_MARKER_END();

    // Go through views and call before view
    for (IndexT i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        if (!view->enabled)
            continue;

        this->currentView = view;
        N_MARKER_BEGIN(ContextPerView, Graphics);
        for (auto& call : this->postLogicViewCalls)
        {
            call(view, this->frameContext);
        }
        N_MARKER_END();
        this->currentView = nullptr;
    }

    // Consider this whole block of code viable for updating resource tables
    CoreGraphics::ResourceTableBlock(true);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::Render()
{
    N_SCOPE(RenderViews, Graphics);
    IndexT i;

    // Go through views and call before view
    for (i = 0; i < this->views.Size(); i++)
    {
        const Ptr<View>& view = this->views[i];

        if (!view->enabled)
            continue;

        this->currentView = view;
        view->Render(this->frameContext.frameIndex, this->frameContext.time, this->frameContext.bufferIndex);
        this->currentView = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::EndFrame()
{
    N_SCOPE(EndFrame, Graphics);
    CoreGraphics::FinishFrame(this->frameContext.frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::NewFrame()
{
    // Wait and get new frame
    CoreGraphics::NewFrame();
}

} // namespace Graphics
