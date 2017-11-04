//------------------------------------------------------------------------------
// graphicsserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsserver.h"
#include "graphicscontext.h"
#include "view.h"
#include "stage.h"
#include "coregraphics/memoryvertexbufferpool.h"
#include "coregraphics/memoryindexbufferpool.h"
#include "resources/resourcemanager.h"
#include "coregraphics/coregraphics.h"
#include "coregraphics/vertexsignaturepool.h"
#include "coregraphics/texturepool.h"
#include "coregraphics/shaderpool.h"
#include "coregraphics/meshpool.h"
#include "models/modelpool.h"

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
	this->visServer = Visibility::VisibilityServer::Create();
	this->timer = FrameSync::FrameSyncTimer::Create();
	this->isOpen = true;

	// register index and vertex buffer pools
	Resources::ResourceManager::Instance()->RegisterMemoryPool(CoreGraphics::MemoryVertexBufferPool::RTTI);
	Resources::ResourceManager::Instance()->RegisterMemoryPool(CoreGraphics::MemoryIndexBufferPool::RTTI);
	Resources::ResourceManager::Instance()->RegisterMemoryPool(CoreGraphics::VertexSignaturePool::RTTI);
	Resources::ResourceManager::Instance()->RegisterStreamPool("dds", CoreGraphics::TexturePool::RTTI);
	Resources::ResourceManager::Instance()->RegisterStreamPool("shd", CoreGraphics::ShaderPool::RTTI);
	Resources::ResourceManager::Instance()->RegisterStreamPool("n3", Models::ModelPool::RTTI);
	Resources::ResourceManager::Instance()->RegisterStreamPool("nvx", CoreGraphics::MeshPool::RTTI);

	// setup internal pool pointers for convenient access (note, will also assert if texture, shader, model or mesh pools is not registered yet!)
	CoreGraphics::vboPool = Resources::GetMemoryPool<CoreGraphics::MemoryVertexBufferPool>();
	CoreGraphics::iboPool = Resources::GetMemoryPool<CoreGraphics::MemoryIndexBufferPool>();
	CoreGraphics::layoutPool = Resources::GetMemoryPool<CoreGraphics::VertexSignaturePool>();

	CoreGraphics::texturePool = Resources::GetStreamPool<CoreGraphics::TexturePool>();
	CoreGraphics::shaderPool = Resources::GetStreamPool<CoreGraphics::ShaderPool>();
	CoreGraphics::modelPool = Resources::GetStreamPool<CoreGraphics::ModelPool>();
	CoreGraphics::meshPool = Resources::GetStreamPool<CoreGraphics::MeshPool>();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::Close()
{
	n_assert(this->isOpen);
	this->visServer = nullptr;
	this->timer = nullptr;
	this->isOpen = false;

	// clear transforms pool
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::OnFrame()
{
	const IndexT frameIndex = this->timer->GetFrameIndex();
	const Timing::Time time = this->timer->GetFrameTime();

	// enter visibility lockstep
	this->visServer->EnterVisibilityLockstep();

	// begin updating visibility
	IndexT i;
	for (i = 0; i < this->contexts.Size(); i++)
	{
		this->contexts[i]->OnBeforeFrame(frameIndex, time);
	}

	// begin updating visibility
	this->visServer->BeginVisibility();

	// go through views and call before view
	for (i = 0; i < this->views.Size(); i++)
	{
		const Ptr<View>& view = this->views[i];
		IndexT j;
		for (j = 0; j < this->contexts.Size(); j++)
		{
			this->contexts[j]->OnBeforeView(view, frameIndex, time);
		}
	}

	for (i = 0; i < this->views.Size(); i++)
	{
		const Ptr<View>& view = this->views[i];

		// apply visibility result for this view
		this->visServer->ApplyVisibility(view);

		IndexT j;
		for (j = 0; j < this->contexts.Size(); j++)
		{
			this->contexts[j]->OnVisibilityReady(frameIndex, time);
		}

		// render view
		view->Render(frameIndex, time);

		for (j = 0; j < this->contexts.Size(); j++)
		{
			this->contexts[j]->OnAfterView(view, frameIndex, time);
		}
	}


	// begin updating visibility
	IndexT i;
	for (i = 0; i < this->contexts.Size(); i++)
	{
		this->contexts[i]->OnAfterFrame(frameIndex, time);
	}

	// leave visibility lockstep
	this->visServer->LeaveVisibilityLockstep();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::RegisterGraphicsContext(const Core::Rtti& rtti)
{
	n_assert(rtti.IsDerivedFrom(GraphicsContext::RTTI));
	Core::RefCounted* obj = rtti.Create();
	Ptr<GraphicsContext> ptr((GraphicsContext*)obj);
	this->contexts.Append(ptr);
}

//------------------------------------------------------------------------------
/**
*/
EntityId
GraphicsServer::CreateGraphicsEntity()
{
	EntityId id;
	return this->entityPool.Allocate(id);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::DiscardGraphicsEntity(const EntityId id)
{
	this->entityPool.Deallocate(id);
}

//------------------------------------------------------------------------------
/**
*/
bool
GraphicsServer::IsValidGraphicsEntity(const EntityId id)
{
	return this->entityPool.IsValid(id);
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
GraphicsServer::CreateView(const Util::StringAtom& framescript)
{
	Ptr<View> view = View::Create();
	this->views.Append(view);
	return view;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsServer::DiscardView(const Ptr<View>& view)
{
	IndexT i = this->views.FindIndex(view);
	n_assert(i != InvalidIndex);
	this->views.EraseIndex(i);
}

} // namespace Graphics