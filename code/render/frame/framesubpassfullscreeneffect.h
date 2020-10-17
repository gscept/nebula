#pragma once
//------------------------------------------------------------------------------
/**
	Performs a full screen draw.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/shader.h"
#include "coregraphics/resourcetable.h"
namespace Frame
{
class FrameSubpassFullscreenEffect : public FrameOp
{
public:
	/// constructor
	FrameSubpassFullscreenEffect();
	/// destructor
	virtual ~FrameSubpassFullscreenEffect();

	/// setup
	void Setup();
	/// discard operation
	void Discard();
    /// Resize render texture
    void OnWindowResized() override;

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex, const IndexT bufferIndex) override;

		CoreGraphics::ShaderProgramId program;
		CoreGraphics::ResourceTableId resourceTable;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);
	
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId> constantBuffers;
	Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId, CoreGraphics::TextureId>> textures;
	CoreGraphics::ResourceTableId resourceTable;

	CoreGraphics::ShaderId shader;
	CoreGraphics::ShaderProgramId program;
	CoreGraphics::TextureId tex;
	
};

} // namespace Frame2
