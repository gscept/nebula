#pragma once
//------------------------------------------------------------------------------
/**
	Executes compute shader.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shader.h"
namespace Frame
{
class FrameCompute : public FrameOp
{
public:
	/// constructor
	FrameCompute();
	/// destructor
	virtual ~FrameCompute();

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::ShaderProgramId program;
	CoreGraphics::ShaderStateId state;
	SizeT x, y, z;
};

} // namespace Frame2