#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used for computations.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "frame/plugins/frameplugin.h"
namespace Frame
{
class FramePluginOp : public FrameOp
{
public:
	/// constructor
	FramePluginOp();
	/// destructor
	virtual ~FramePluginOp();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);
		void Discard();

		std::function<void(IndexT)> func;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

	std::function<void(IndexT)> func;
private:


	void Build(
		Memory::ArenaAllocator<BIG_CHUNK>& allocator,
		Util::Array<FrameOp::Compiled*>& compiledOps,
		Util::Array<CoreGraphics::EventId>& events,
		Util::Array<CoreGraphics::BarrierId>& barriers,
		Util::Dictionary<CoreGraphics::ShaderRWBufferId, Util::Array<BufferDependency>>& rwBuffers,
		Util::Dictionary<CoreGraphics::TextureId, Util::Array<TextureDependency>>& textures) override;
};

} // namespace Frame2