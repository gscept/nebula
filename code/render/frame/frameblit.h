#pragma once
//------------------------------------------------------------------------------
/**
	A frame blit performs an image copy with optional filtering and image type conversions.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/rendertexture.h"
namespace Frame
{
class FrameBlit : public FrameOp
{
public:
	/// constructor
	FrameBlit();
	/// destructor
	virtual ~FrameBlit();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);
		void Discard();

#if defined(NEBULA_GRAPHICS_DEBUG)
		Util::StringAtom name;
#endif

		CoreGraphics::RenderTextureId from, to;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);

	CoreGraphics::RenderTextureId from, to;
};

} // namespace Frame2