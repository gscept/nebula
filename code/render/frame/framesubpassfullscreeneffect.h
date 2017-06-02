#pragma once
//------------------------------------------------------------------------------
/**
	Performs a full screen draw.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "renderutil/drawfullscreenquad.h"
#include "coregraphics/shaderstate.h"
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
	
	/// set render texture the fullscreen effect should use as reference
	void SetRenderTexture(const Ptr<CoreGraphics::RenderTexture>& tex);
	/// set shader state
	void SetShaderState(const Ptr<CoreGraphics::ShaderState>& shaderState);

private:
	RenderUtil::DrawFullScreenQuad fsq;
	Ptr<CoreGraphics::ShaderState> shaderState;
	Ptr<CoreGraphics::RenderTexture> tex;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassFullscreenEffect::SetRenderTexture(const Ptr<CoreGraphics::RenderTexture>& tex)
{
	this->tex = tex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassFullscreenEffect::SetShaderState(const Ptr<CoreGraphics::ShaderState>& shaderState)
{
	this->shaderState = shaderState;
}

} // namespace Frame2