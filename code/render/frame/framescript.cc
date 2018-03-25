//------------------------------------------------------------------------------
// framescript.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescript.h"
#include "frameserver.h"
#include "coregraphics/displaydevice.h"


namespace Frame
{

__ImplementClass(Frame::FrameScript, 'FRSC', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
FrameScript::FrameScript() :
	window(Ids::InvalidId32)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameScript::~FrameScript()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddColorTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex)
{
	n_assert(!this->colorTexturesByName.Contains(name));
	this->colorTexturesByName.Add(name, tex);
	this->colorTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddDepthStencilTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex)
{
	n_assert(!this->depthStencilTexturesByName.Contains(name));
	this->depthStencilTexturesByName.Add(name, tex);
	this->depthStencilTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddReadWriteTexture(const Util::StringAtom& name, const CoreGraphics::ShaderRWTextureId tex)
{
	n_assert(!this->readWriteTexturesByName.Contains(name));
	this->readWriteTexturesByName.Add(name, tex);
	this->readWriteTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddReadWriteBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId buf)
{
	n_assert(!this->readWriteBuffersByName.Contains(name));
	this->readWriteBuffersByName.Add(name, buf);
	this->readWriteBuffers.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddEvent(const Util::StringAtom& name, const CoreGraphics::EventId event)
{
	n_assert(!this->eventsByName.Contains(name));
	this->eventsByName.Add(name, event);
	this->events.Append(event);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddAlgorithm(const Util::StringAtom& name, Algorithms::Algorithm* alg)
{
	n_assert(!this->algorithmsByName.Contains(name));
	this->algorithmsByName.Add(name, alg);
	this->algorithms.Append(alg);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddShaderState(const Util::StringAtom& name, const CoreGraphics::ShaderStateId state)
{
	n_assert(!this->shaderStatesByName.Contains(name));
	this->shaderStatesByName.Add(name, state);
	this->shaderStates.Append(state);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddOp(Frame::FrameOp* op)
{
	this->ops.Append(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Setup()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Discard()
{
	// unload ourselves, this is only for convenience
	FrameServer::Instance()->UnloadFrameScript(this->resId);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Run(const IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Run(frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::RunSegment(const FrameOp::ExecutionMask mask, const IndexT frameIndex)
{
	IndexT start = mask & 0x000000FF;
	IndexT end = (mask & 0x0000FF00) >> 8;
	IndexT i;
	for (i = start; i < end; i++)
	{
		this->ops[i]->Run(frameIndex);
	}
}

//------------------------------------------------------------------------------
/**
	Create a mask to run a script between two operations.
*/
Frame::FrameOp::ExecutionMask
FrameScript::CreateMask(const Util::StringAtom& startOp, const Util::StringAtom& endOp)
{
	FrameOp::ExecutionMask mask = 0;
	IndexT i;
	n_assert(this->ops.Size() < 256);
	for (i = 0; i < this->ops.Size(); i++)
	{
		FrameOp* op = this->ops[i];
		if (op->GetName() == startOp)
		{
			mask |= i & 0x000000FF;
		}
		else if (op->GetName() == endOp)
		{
			mask |= (i << 8) & 0x0000FF00;
		}
	}
	n_assert((mask & 0x000000FF) < ((mask & 0x0000FF00) >> 8));
	return mask;
}

//------------------------------------------------------------------------------
/**
	Create a mask to run from one subpass to another, from within a pass.
*/
Frame::FrameOp::ExecutionMask
FrameScript::CreateSubpassMask(const Ptr<FramePass>& pass, const Util::StringAtom& startOp, const Util::StringAtom& endOp)
{
	FrameOp::ExecutionMask mask = 0;
	const Util::Array<FrameSubpass*>& subpasses = pass->GetSubpasses();
	n_assert(subpasses.Size() < 256);
	IndexT i;
	for (i = 0; i < subpasses.Size(); i++)
	{
		FrameSubpass* subpass = subpasses[i];
		if (subpass->GetName() == startOp)
		{
			mask |= (i << 12) & 0x00FF0000;
		}
		else if (subpass->GetName() == endOp)
		{
			mask |= (i << 16) & 0xFF000000;
		}
		
	}
	n_assert(((mask & 0x00FF0000) >> 12) < ((mask & 0xFF000000) >> 16));
	return mask;
}

//------------------------------------------------------------------------------
/**
	Get operation start and end points from within script.
*/
void
FrameScript::GetOps(const FrameOp::ExecutionMask mask, FrameOp* startOp, FrameOp* endOp)
{
	IndexT start = mask & 0x000000FF;
	IndexT end = (mask & 0x0000FF00) >> 8;
	startOp = this->ops[start];
	endOp = this->ops[end];
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Cleanup()
{
	IndexT i;
	for (i = 0; i < this->colorTextures.Size(); i++) DestroyRenderTexture(this->colorTextures[i]);
	this->colorTextures.Clear();
	this->colorTexturesByName.Clear();

	for (i = 0; i < this->depthStencilTextures.Size(); i++) DestroyRenderTexture(this->depthStencilTextures[i]);
	this->depthStencilTextures.Clear();
	this->depthStencilTexturesByName.Clear();

	for (i = 0; i < this->readWriteTextures.Size(); i++) DestroyShaderRWTexture(this->readWriteTextures[i]);
	this->readWriteTextures.Clear();
	this->readWriteTexturesByName.Clear();

	for (i = 0; i < this->readWriteBuffers.Size(); i++) DestroyShaderRWBuffer(this->readWriteBuffers[i]);
	this->readWriteBuffers.Clear();
	this->readWriteBuffersByName.Clear();

	for (i = 0; i < this->events.Size(); i++) DestroyEvent(this->events[i]);
	this->events.Clear();
	this->eventsByName.Clear();

	for (i = 0; i < this->shaderStates.Size(); i++) ShaderDestroyState(this->shaderStates[i]);
	this->shaderStates.Clear();
	this->shaderStatesByName.Clear();

	for (i = 0; i < this->algorithms.Size(); i++) this->algorithms[i]->Discard();
	this->algorithms.Clear();
	this->algorithmsByName.Clear();

	for (i = 0; i < this->ops.Size(); i++) this->ops[i]->Discard();
	this->ops.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::OnWindowResized()
{
	// only do this if we actually use the window
	if (this->window != Ids::InvalidId32)
	{
		CoreGraphics::WindowId prev = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();

		// make this window current
		WindowMakeCurrent(this->window);

		IndexT i;
		for (i = 0; i < this->colorTextures.Size(); i++)		RenderTextureWindowResized(this->colorTextures[i]);
		for (i = 0; i < this->readWriteTextures.Size(); i++)	ShaderRWTextureWindowResized(this->readWriteTextures[i]);
		for (i = 0; i < this->algorithms.Size(); i++)			this->algorithms[i]->Resize();
		for (i = 0; i < this->ops.Size(); i++)					this->ops[i]->OnWindowResized();

		// reset old window
		WindowMakeCurrent(prev);
	}

}

} // namespace Frame2