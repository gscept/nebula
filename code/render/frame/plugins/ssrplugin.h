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
	void Setup() override;
	/// discard
	void Discard() override;

	/// update resources
	void UpdateResources(const IndexT frameIndex) override;
	/// handle window resizing
	void Resize() override;

private:
    CoreGraphics::ShaderId traceShader;
	Util::FixedArray<CoreGraphics::ResourceTableId> ssrTraceTables;
    IndexT traceBufferSlot;
    IndexT constantsSlot;

    CoreGraphics::ShaderId resolveShader;
	Util::FixedArray<CoreGraphics::ResourceTableId> ssrResolveTables;
	IndexT reflectionBufferSlot;
	IndexT lightBufferSlot;
	IndexT resolveTraceBufferSlot;

	CoreGraphics::ShaderProgramId traceProgram;
	CoreGraphics::ShaderProgramId resolveProgram;

	CoreGraphics::ConstantBufferId constants;
};

} // namespace Algorithms
