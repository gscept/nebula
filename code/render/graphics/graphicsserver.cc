//------------------------------------------------------------------------------
// graphicsserver.cc
// (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "graphicsserver.h"
#include "graphicscontext.h"
#include "view.h"
#include "stage.h"
#include "resources/resourcemanager.h"
#include "coregraphics/vertexsignaturepool.h"
#include "coregraphics/streamtexturepool.h"
#include "coregraphics/memorytexturepool.h"
#include "coregraphics/memorymeshpool.h"
#include "coregraphics/shaderpool.h"
#include "coregraphics/streammeshpool.h"
#include "coreanimation/streamanimationpool.h"
#include "characters/streamskeletonpool.h"
#include "models/streammodelpool.h"

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
	this->timer = FrameSync::FrameSyncTimer::Create();
	this->isOpen = true;

	this->displayDevice = CoreGraphics::DisplayDevice::Create();
	this->displayDevice->Open();

	static const SizeT MB = 1024 * 1024;
	CoreGraphics::GraphicsDeviceCreateInfo gfxInfo{ 
		{ 1_MB, 50_MB },		// Graphics - main threads get 1 MB of constant memory, visibility thread (objects) gets 50
		{ 1_MB, 0_MB },			// Compute - main threads get 1 MB of constant memory, visibility thread (objects) gets 0
		{ 10_MB, 1_MB },        // Vertex memory - main thread gets 10 MB for UI, Text etc, visibility thread (objects doing soft cloths and such) get 1 MB
		{ 5_MB, 1_MB },         // Index memory - main thread gets 5 MB for UI, Text etc, visibility thread (objects doing soft cloths and such) get 1 MB
		2,						// Number of simultaneous frames (N buffering)
		false }; // validation
	this->graphicsDevice = CoreGraphics::CreateGraphicsDevice(gfxInfo);
	if (this->graphicsDevice)
	{

		// register graphics context pools
		Resources::ResourceManager::Instance()->RegisterMemoryPool(CoreGraphics::VertexSignaturePool::RTTI);
		Resources::ResourceManager::Instance()->RegisterMemoryPool(CoreGraphics::MemoryTexturePool::RTTI);
		Resources::ResourceManager::Instance()->RegisterMemoryPool(CoreGraphics::MemoryMeshPool::RTTI);

		Resources::ResourceManager::Instance()->RegisterStreamPool("dds", CoreGraphics::StreamTexturePool::RTTI);
		Resources::ResourceManager::Instance()->RegisterStreamPool("fxb", CoreGraphics::ShaderPool::RTTI);
		Resources::ResourceManager::Instance()->RegisterStreamPool("nax3", CoreAnimation::StreamAnimationPool::RTTI);
		Resources::ResourceManager::Instance()->RegisterStreamPool("nsk3", Characters::StreamSkeletonPool::RTTI); 
		Resources::ResourceManager::Instance()->RegisterStreamPool("nvx2", CoreGraphics::StreamMeshPool::RTTI);
		Resources::ResourceManager::Instance()->RegisterStreamPool("sur", Materials::SurfacePool::RTTI);
		Resources::ResourceManager::Instance()->RegisterStreamPool("n3", Models::StreamModelPool::RTTI);

		// setup internal pool pointers for convenient access (note, will also assert if texture, shader, model or mesh pools is not registered yet!)
		CoreGraphics::layoutPool = Resources::GetMemoryPool<CoreGraphics::VertexSignaturePool>();
		CoreGraphics::texturePool = Resources::GetMemoryPool<CoreGraphics::MemoryTexturePool>();
		CoreGraphics::meshPool = Resources::GetMemoryPool<CoreGraphics::MemoryMeshPool>();

		CoreGraphics::shaderPool = Resources::GetStreamPool<CoreGraphics::ShaderPool>();
		Models::modelPool = Resources::GetStreamPool<Models::StreamModelPool>();
		Materials::surfacePool = Resources::GetStreamPool<Materials::SurfacePool>();

		CoreAnimation::animPool = Resources::GetStreamPool<CoreAnimation::StreamAnimationPool>();
		Characters::skeletonPool = Resources::GetStreamPool<Characters::StreamSkeletonPool>();

		// load base textures before setting up major subsystems
		const unsigned char white = 0xFF;
		const unsigned char black = 0x00;
		CoreGraphics::TextureCreateInfo texInfo;
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

		this->shaderServer = CoreGraphics::ShaderServer::Create();
		this->shaderServer->Open();

		this->frameServer = Frame::FrameServer::Create();
		this->frameServer->Open();

		this->materialServer = Materials::MaterialServer::Create();
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
		Resources::ResourceManager::Instance()->LoadDefaultResources();
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

	if (this->graphicsDevice) CoreGraphics::DestroyGraphicsDevice();

	// clear transforms pool
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
	this->timer->UpdateTimePolling();
	this->frameIndex = this->timer->GetFrameIndex();
	this->frameTime = this->timer->GetFrameTime();
	this->time = this->timer->GetTime();
	this->ticks = this->timer->GetTicks();

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

	// go through views and call prepare view
	for (i = 0; i < this->views.Size(); i++)
	{
		const Ptr<View>& view = this->views[i];

		IndexT j;
		for (j = 0; j < this->contexts.Size(); j++)
		{
			if (this->contexts[j]->StageBits)
				*this->contexts[j]->StageBits = Graphics::OnPrepareViewStage;
			if (this->contexts[j]->OnPrepareView != nullptr)
				this->contexts[j]->OnPrepareView(view, this->frameIndex, this->frameTime);
		}		
	}

	// begin frame
	CoreGraphics::BeginFrame(this->frameIndex);

	for (i = 0; i < this->contexts.Size(); i++)
	{
		if (this->contexts[i]->StageBits)
			*this->contexts[i]->StageBits = Graphics::OnBeforeFrameStage;

		if (this->contexts[i]->OnBeforeFrame != nullptr)
			this->contexts[i]->OnBeforeFrame(this->frameIndex, this->frameTime, this->time, this->ticks);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::BeforeViews()
{
	// wait for visibility
	IndexT i;
	for (i = 0; i < this->contexts.Size(); i++)
	{
		if (this->contexts[i]->StageBits)
			*this->contexts[i]->StageBits = Graphics::OnWaitForWorkStage;
		if (this->contexts[i]->OnWaitForWork != nullptr)
			this->contexts[i]->OnWaitForWork(this->frameIndex, this->frameTime);
	}

	// go through views and call before view
	for (i = 0; i < this->views.Size(); i++)
	{
		const Ptr<View>& view = this->views[i];
		
		if (!view->enabled)
			continue;

		this->currentView = view;

		// begin frame
		this->currentView->BeginFrame(this->frameIndex, this->frameTime);
		this->shaderServer->BeforeView();

		IndexT j;
		for (j = 0; j < this->contexts.Size(); j++)
		{
			if (this->contexts[i]->StageBits)
				*this->contexts[i]->StageBits = Graphics::OnBeforeViewStage;
			if (this->contexts[j]->OnBeforeView != nullptr)
				this->contexts[j]->OnBeforeView(view, this->frameIndex, this->frameTime);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::RenderViews()
{
	IndexT i;
	// go through views and call before view
	for (i = 0; i < this->views.Size(); i++)
	{
		const Ptr<View>& view = this->views[i];

		if (!view->enabled)
			continue;

		view->Render(this->frameIndex, this->frameTime);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::EndViews()
{
	// go through views and call before view
	IndexT i;
	for (i = 0; i < this->views.Size(); i++)
	{
		const Ptr<View>& view = this->views[i];

		if (!view->enabled)
			continue;

		this->shaderServer->AfterView();
		this->currentView->EndFrame(this->frameIndex, this->frameTime);

		IndexT j;
		for (j = 0; j < this->contexts.Size(); j++)
		{
			if (this->contexts[i]->StageBits)
				*this->contexts[i]->StageBits = Graphics::OnAfterViewStage;
			if (this->contexts[j]->OnAfterView != nullptr)
				this->contexts[j]->OnAfterView(view, this->frameIndex, this->frameTime);
		}
	}

	this->currentView = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::EndFrame()
{

	// stop the graphics side frame
	CoreGraphics::EndFrame(this->frameIndex);

	// finish frame and prepare for the next one
	IndexT i;
	for (i = 0; i < this->contexts.Size(); i++)
	{
		if (this->contexts[i]->StageBits) 
			*this->contexts[i]->StageBits = Graphics::OnAfterFrameStage;
		if (this->contexts[i]->OnAfterFrame != nullptr)
			this->contexts[i]->OnAfterFrame(this->frameIndex, this->frameTime);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
GraphicsServer::RenderPlugin(const Util::StringAtom & filter)
{
    // finish frame and prepare for the next one
    IndexT i;
    for (i = 0; i < this->contexts.Size(); i++)
    {
        if (this->contexts[i]->OnRenderAsPlugin != nullptr)
            this->contexts[i]->OnRenderAsPlugin(this->frameIndex, this->frameTime, filter);
    }
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

	// setup gbuffer bindings after frame script is loaded
	this->shaderServer->SetupGBufferConstants();

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

	// setup gbuffer bindings after frame script is loaded
	this->shaderServer->SetupGBufferConstants();

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