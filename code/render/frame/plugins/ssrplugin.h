#pragma once
//------------------------------------------------------------------------------
/**
	Implements screen space reflections as a script plugin
	
	(C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameplugin.h"
#include "coregraphics/shader.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/constantbuffer.h"
// #include "renderutil/drawfullscreenquad.h"

namespace Frame
{
class SSRPlugin : public FramePlugin
{
public:
	/// constructor
    SSRPlugin();
	/// destructor
	virtual ~SSRPlugin();

	/// setup
	void Setup();
	/// discard
	void Discard();

	/// handle window resizing
	void Resize();

private:
    CoreGraphics::ShaderId shader;
	Util::FixedArray<CoreGraphics::ResourceTableId> ssrTables;
    IndexT traceBufferSlot;
    IndexT constantsSlot;

	// CoreGraphics::ConstantBinding zThicknessBinding;
	// CoreGraphics::ConstantBinding pixelStrideBinding;
	// CoreGraphics::ConstantBinding maxStepsBinding;
    // CoreGraphics::ConstantBinding maxDistanceBinding;
    // CoreGraphics::ConstantBinding ViewToTextureSpaceBinding;

	CoreGraphics::ShaderProgramId program;

	CoreGraphics::ConstantBufferId constants;
};

} // namespace Algorithms
