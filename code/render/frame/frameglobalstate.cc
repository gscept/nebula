//------------------------------------------------------------------------------
// frameglobalstate.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "frameglobalstate.h"

namespace Frame
{

__ImplementClass(Frame::FrameGlobalState, 'FRGS', Frame::FrameOp);
//------------------------------------------------------------------------------
/**
*/
FrameGlobalState::FrameGlobalState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameGlobalState::~FrameGlobalState()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::AddVariableInstance(const Ptr<CoreGraphics::ShaderVariableInstance>& var)
{
	this->variableInstances.Append(var);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::Discard()
{
	FrameOp::Discard();

	IndexT i;
	for (i = 0; i < this->variableInstances.Size(); i++) this->variableInstances[i]->Discard();
	this->variableInstances.Clear();
	this->state->Discard();
	this->state = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameGlobalState::Run(const IndexT frameIndex)
{
	// apply variable instances
	IndexT i;
	for (i = 0; i < this->variableInstances.Size(); i++)
	{
		this->variableInstances[i]->Apply();
	}

	// then commit
	this->state->Apply();
	this->state->Commit();
}

} // namespace Frame2