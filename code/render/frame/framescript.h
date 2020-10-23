#pragma once
//------------------------------------------------------------------------------
/**
	A FrameScript describes render operations being done to produce a single frame.

	Frame scripts are loaded once like a template, and then compiled to produce
	an optimized result. When a pass is disabled or re-enabled, the script
	is rebuilt, so refrain from doing this frequently. 

	On DX12 and Vulkan, the compile process serves to insert proper barriers, events
	and semaphore operations such that shader resources are not stomped or read prematurely.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/texture.h"
#include "coregraphics/buffer.h"
#include "coregraphics/event.h"
#include "coregraphics/shader.h"
#include "frame/frameop.h"
#include "frame/framepass.h"
#include "memory/arenaallocator.h"
#include "threading/event.h"
namespace Graphics
{
	class View;
}

namespace CoreGraphics
{
	class DrawThread;
}
namespace Frame
{
class FrameScript : public Core::RefCounted
{
	__DeclareClass(FrameScript);
public:
	/// constructor
	FrameScript();
	/// destructor
	virtual ~FrameScript();

	/// get allocator
	Memory::ArenaAllocator<BIG_CHUNK>& GetAllocator();

	/// set name
	void SetResourceName(const Resources::ResourceName& name);
	/// get name
	const Resources::ResourceName& GetResourceName() const;

	/// add frame operation
	void AddOp(Frame::FrameOp* op);
	/// add texture
	void AddTexture(const Util::StringAtom& name, const CoreGraphics::TextureId tex);
	/// get texture
	const CoreGraphics::TextureId GetTexture(const Util::StringAtom& name);
	/// get all textures
	const Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId>& GetTextures() const;
	/// add buffer
	void AddBuffer(const Util::StringAtom& name, const CoreGraphics::BufferId buf);
	/// get buffer
	const CoreGraphics::BufferId GetBuffer(const Util::StringAtom& name);

	/// setup script
	void Setup();
	/// discard script
	void Discard();
	/// run through script and generate thread jobs where applicable
	void RunJobs(const IndexT frameIndex);
	/// run script
	void Run(const IndexT frameIndex, const IndexT bufferIndex);

	/// build framescript, this will delete and replace the old frame used for Run()
	void Build();

private:
	friend class FrameScriptLoader;
	friend class FrameServer;
	friend class Graphics::View;

	/// cleanup script internally
	void Cleanup();
	/// handle resizing
	void OnWindowResized();

	CoreGraphics::WindowId window;
	Memory::ArenaAllocator<BIG_CHUNK> allocator;

	Util::Array<CoreGraphics::EventId> events;
	Util::Array<CoreGraphics::BarrierId> barriers;
	Memory::ArenaAllocator<BIG_CHUNK> buildAllocator;

	Resources::ResourceName resId;

	Util::Array<CoreGraphics::BufferId> buffers;
	Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId> buffersByName;
	Util::Array<CoreGraphics::TextureId> textures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId> texturesByName;

	Util::Array<Frame::FrameOp*> ops;
	Util::Array<Frame::FrameOp::Compiled*> compiled;
	Util::Array<CoreGraphics::BarrierId> resourceResetBarriers;
	IndexT frameOpCounter;

	Ptr<CoreGraphics::DrawThread> drawThread;
	Threading::Event drawThreadEvent;
	CoreGraphics::CommandBufferPoolId drawThreadCommandPool;

	bool subScript; // if subscript, it means it can only be ran from within another script
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameScript::SetResourceName(const Resources::ResourceName& name)
{
	this->resId = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
FrameScript::GetResourceName() const
{
	return this->resId;
}

//------------------------------------------------------------------------------
/**
*/
inline Memory::ArenaAllocator<BIG_CHUNK>&
FrameScript::GetAllocator()
{
	return this->allocator;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::TextureId 
FrameScript::GetTexture(const Util::StringAtom& name)
{
	return this->texturesByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId>&
FrameScript::GetTextures() const
{
	return this->texturesByName;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::BufferId 
FrameScript::GetBuffer(const Util::StringAtom& name)
{
	return this->buffersByName[name];
}

} // namespace Frame
