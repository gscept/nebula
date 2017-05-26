#pragma once
//------------------------------------------------------------------------------
/**
	Implements a global state which is initialized at the beginning of the frame.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/shaderstate.h"
namespace Frame2
{
class FrameGlobalState : public FrameOp
{
	__DeclareClass(FrameGlobalState);
public:
	/// constructor
	FrameGlobalState();
	/// destructor
	virtual ~FrameGlobalState();

	/// set state to use
	void SetShaderState(const Ptr<CoreGraphics::ShaderState>& state);
	/// add variable instance
	void AddVariableInstance(const Ptr<CoreGraphics::ShaderVariableInstance>& var);

	/// discard operation
	void Discard();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	Ptr<CoreGraphics::ShaderState> state;
	Util::Array<Ptr<CoreGraphics::ShaderVariableInstance>> variableInstances;
};


//------------------------------------------------------------------------------
/**
*/
inline void
FrameGlobalState::SetShaderState(const Ptr<CoreGraphics::ShaderState>& state)
{
	this->state = state;
}

} // namespace Frame2