#pragma once
//------------------------------------------------------------------------------
/**
	A frame pass prepares a rendering sequence, draws and subpasses must reside
	within one of these objects.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "framesubpass.h"
#include "coregraphics/pass.h"
namespace Threading
{
class Event;
}

namespace Frame
{
class FramePass : public FrameOp
{
public:
	/// constructor
	FramePass();
	/// destructor
	virtual ~FramePass();

	/// add subpass
	void AddSubpass(FrameSubpass* subpass);

	/// get list of subpasses
	const Util::Array<FrameSubpass*>& GetSubpasses() const;

	/// discard operation
	void Discard();
	/// handle display resizing
	void OnWindowResized() override;

	struct CompiledImpl : public FrameOp::Compiled
	{
		void RunJobs(const IndexT frameIndex);
		void Run(const IndexT frameIndex);
		void Discard();

#if NEBULA_GRAPHICS_DEBUG
		Util::StringAtom name;
#endif
		Util::Array<FrameOp::Compiled*> subpasses;
		Util::Array<Util::FixedArray<CoreGraphics::CommandBufferId>> subpassBuffers;
		CoreGraphics::PassId pass;
	};

	/// allocate new instance
	FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

	CoreGraphics::PassId pass;

private:
	friend class FrameScript;

	void Build(
		Memory::ArenaAllocator<BIG_CHUNK>& allocator,
		Util::Array<FrameOp::Compiled*>& compiledOps,
		Util::Array<CoreGraphics::EventId>& events,
		Util::Array<CoreGraphics::BarrierId>& barriers,
		Util::Dictionary<CoreGraphics::BufferId, Util::Array<BufferDependency>>& rwBuffers,
		Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures) override;

	Util::Array<FrameSubpass*> subpasses;
};

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Frame::FrameSubpass*>&
FramePass::GetSubpasses() const
{
	return this->subpasses;
}

} // namespace Frame2