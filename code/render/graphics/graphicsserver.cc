//------------------------------------------------------------------------------
// graphicsserver.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicsserver.h"
#include "graphicscontext.h"
#include "view.h"

namespace Graphics
{

__ImplementClass(Graphics::GraphicsServer, 'GFXS', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
GraphicsServer::GraphicsServer() :
	isOpen(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
GraphicsServer::~GraphicsServer()
{
	// empty
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