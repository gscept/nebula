#pragma once
//------------------------------------------------------------------------------
/**
	Performs a full screen draw.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/shader.h"
#include "coregraphics/rendertexture.h"
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

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		RenderUtil::DrawFullScreenQuad fsq;
		CoreGraphics::ShaderProgramId program;
		CoreGraphics::ResourceTableId resourceTable;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);
	
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId> constantBuffers;
	CoreGraphics::ResourceTableId resourceTable;

	RenderUtil::DrawFullScreenQuad fsq;
	CoreGraphics::ShaderId shader;
	CoreGraphics::ShaderProgramId program;
	CoreGraphics::RenderTextureId tex;
	
};

} // namespace Frame2