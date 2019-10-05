#pragma once
//------------------------------------------------------------------------------
/**
	Performs an algorithm used for computations.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "algorithm/algorithm.h"
namespace Frame
{
class FrameComputeAlgorithm : public FrameOp
{
public:
	/// constructor
	FrameComputeAlgorithm();
	/// destructor
	virtual ~FrameComputeAlgorithm();

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
		Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& rwTextures,
		Util::Dictionary<CoreGraphics::ShaderRWBufferId, BufferDependency>& rwBuffers,
		Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, TextureDependency>>>& renderTextures) override;
};

} // namespace Frame2