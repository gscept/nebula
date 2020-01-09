//------------------------------------------------------------------------------
// framescript.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
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
	window(Ids::InvalidId32),
	frameOpCounter(0),
	subScript(false)
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
FrameScript::AddTexture(const Util::StringAtom& name, const CoreGraphics::TextureId tex)
{
	IndexT i = this->texturesByName.FindIndex(name);
	n_assert(i == InvalidIndex);
	this->texturesByName.Add(name, tex);
	this->textures.Append(tex);
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
FrameScript::AddPlugin(const Util::StringAtom& name, Frame::FramePlugin* alg)
{
	n_assert(!this->algorithmsByName.Contains(name));
	this->algorithmsByName.Add(name, alg);
	this->plugins.Append(alg);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddOp(Frame::FrameOp* op)
{
	op->index = this->frameOpCounter;
	this->frameOpCounter++;
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

	IndexT i;
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Discard();
		this->ops[i]->~FrameOp();
	}

	this->buildAllocator.Release();
	this->allocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Run(const IndexT frameIndex)
{
	IndexT i;
	for (i = 0; i < this->compiled.Size(); i++)
	{
		this->compiled[i]->QueuePreSync();			// wait within queue
		this->compiled[i]->Run(frameIndex);
		this->compiled[i]->QueuePostSync();			// signal within queue
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Build()
{
	IndexT i;
    
	// go through and discard all compiled (dunno if needed)
    if (!this->compiled.IsEmpty())
    {
	    for (i = 0; i < this->compiled.Size(); i++)
	    {
		    this->compiled[i]->Discard();
	    }
        this->compiled.Clear();
    }

	for (i = 0; i < this->resourceResetBarriers.Size(); i++)
		DestroyBarrier(this->resourceResetBarriers[i]);
	this->resourceResetBarriers.Clear();

	for (i = 0; i < this->events.Size(); i++)
		DestroyEvent(this->events[i]);

	for (i = 0; i < this->barriers.Size(); i++)
		DestroyBarrier(this->barriers[i]);

	// clear old compiled result
	this->buildAllocator.Release();
	 
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<FrameOp::BufferDependency>> rwBuffers;
	Util::Dictionary<CoreGraphics::TextureId, Util::Array<FrameOp::TextureDependency>> textures;

	// give every resource an initial dependency
	for (i = 0; i < this->textures.Size(); i++)
	{
		CoreGraphics::TextureId tex = this->textures[i];
		bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(tex));
		CoreGraphics::ImageLayout layout = CoreGraphics::TextureGetDefaultLayout(tex);
		auto& arr = textures.AddUnique(tex);

		uint layers = CoreGraphics::TextureGetNumLayers(tex);
		uint mips = CoreGraphics::TextureGetNumMips(tex);

		CoreGraphics::ImageSubresourceInfo subres;
		subres.aspect = isDepth ? CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits : CoreGraphics::ImageAspect::ColorBits;
		subres.layer = 0;
		subres.layerCount = layers;
		subres.mip = 0;
		subres.mipCount = mips;
		arr.Append(FrameOp::TextureDependency{ nullptr, CoreGraphics::QueueType::GraphicsQueueType, layout, CoreGraphics::BarrierStage::AllGraphicsShaders | CoreGraphics::BarrierStage::ComputeShader, CoreGraphics::BarrierAccess::ShaderRead, DependencyIntent::Read, InvalidIndex, subres });
	}

	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Build(this->buildAllocator, this->compiled, this->events, this->barriers, rwBuffers, textures);
	}

	// setup a post-frame barrier to reset the resource state of all resources back to their created original (ShaderRead for RenderTexture, General for RWTexture
	Util::Array<CoreGraphics::TextureBarrier> texturesBarr;

	for (i = 0; i < textures.Size(); i++)
	{
		const CoreGraphics::TextureId& res = textures.KeyAtIndex(i);
		CoreGraphics::ImageLayout layout = CoreGraphics::TextureGetDefaultLayout(res);
		const Util::Array<FrameOp::TextureDependency>& deps = textures.ValueAtIndex(i);

		const FrameOp::TextureDependency& dep = deps.Back();
		const CoreGraphics::ImageSubresourceInfo& info = dep.subres;
		CoreGraphics::BarrierAccess outAccess = dep.layout == CoreGraphics::ImageLayout::Present ? CoreGraphics::BarrierAccess::TransferRead : CoreGraphics::BarrierAccess::ShaderRead;
		CoreGraphics::BarrierStage outStage = outAccess == CoreGraphics::BarrierAccess::TransferRead ? CoreGraphics::BarrierStage::Transfer : CoreGraphics::BarrierStage::AllGraphicsShaders;

		// render textures are created as shader read
		if (dep.layout != layout)
		{
			CoreGraphics::BarrierCreateInfo inf =
			{
				Util::String::Sprintf("End of Frame Texture Reset Transition %d", res.resourceId),
				CoreGraphics::BarrierDomain::Global,
				dep.stage,
				outStage,
				{ CoreGraphics::TextureBarrier{ res, info, dep.layout, layout, dep.access, outAccess } }, nullptr
			};
			this->resourceResetBarriers.Append(CoreGraphics::CreateBarrier(inf));
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Cleanup()
{
	IndexT i;
	for (i = 0; i < this->textures.Size(); i++) DestroyTexture(this->textures[i]);
	this->textures.Clear();
	this->texturesByName.Clear();

	for (i = 0; i < this->readWriteBuffers.Size(); i++) DestroyShaderRWBuffer(this->readWriteBuffers[i]);
	this->readWriteBuffers.Clear();
	this->readWriteBuffersByName.Clear();

	for (i = 0; i < this->events.Size(); i++) DestroyEvent(this->events[i]);
	this->events.Clear();

	for (i = 0; i < this->plugins.Size(); i++) this->plugins[i]->Discard();
	this->plugins.Clear();
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
		for (i = 0; i < this->textures.Size(); i++)			TextureWindowResized(this->textures[i]);
		for (i = 0; i < this->plugins.Size(); i++)			this->plugins[i]->Resize();
		for (i = 0; i < this->ops.Size(); i++)				this->ops[i]->OnWindowResized();

        Build();

		// reset old window
		WindowMakeCurrent(prev);
	}

}

} // namespace Frame2
