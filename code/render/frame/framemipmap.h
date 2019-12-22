#pragma once
//------------------------------------------------------------------------------
/**
	Updates mip chain for texture
	
	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{
class FrameMipmap : public FrameOp
{
public:
	/// constructor
	FrameMipmap();
	/// destructor
	virtual ~FrameMipmap();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);
		void Discard();

#if NEBULA_GRAPHICS_DEBUG
		Util::StringAtom name;
#endif

		CoreGraphics::TextureId tex;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);

	CoreGraphics::TextureId tex;
};

} // namespace Frame2