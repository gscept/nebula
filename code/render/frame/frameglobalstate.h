#pragma once
//------------------------------------------------------------------------------
/**
	Implements a global state which is initialized at the beginning of the frame.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shader.h"
namespace Frame
{
class FrameGlobalState : public FrameOp
{
	__DeclareClass(FrameGlobalState);
public:
	/// constructor
	FrameGlobalState();
	/// destructor
	virtual ~FrameGlobalState();

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);

	CoreGraphics::ShaderStateId state;
	Util::Array<CoreGraphics::ShaderConstantId> constants;
};

} // namespace Frame2