#pragma once
//------------------------------------------------------------------------------
/**
	Performs a full screen draw.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/shader.h"
#include "coregraphics/rendertexture.h"
namespace Frame
{
class FrameSubpassFullscreenEffect : public FrameOp
{
	__DeclareClass(FrameSubpassFullscreenEffect);
public:
	/// constructor
	FrameSubpassFullscreenEffect();
	/// destructor
	virtual ~FrameSubpassFullscreenEffect();

	/// setup
	void Setup();
	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
	
	RenderUtil::DrawFullScreenQuad fsq;
	CoreGraphics::ShaderStateId shaderState;
	CoreGraphics::RenderTextureId tex;
	
};

} // namespace Frame2