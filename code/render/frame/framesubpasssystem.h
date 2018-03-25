#pragma once
//------------------------------------------------------------------------------
/**
	A subpass system operation executes a pre-defined subsystem, like lights, UI, text etc.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
namespace Frame
{
class FrameSubpassSystem : public Frame::FrameOp
{
public:
	enum Subsystem
	{
		Lights,
		LightProbes,
		LocalShadowsSpot,
		LocalShadowsPoint,
		GlobalShadows,
		UI,
		Text,
		Shapes,
	};

	/// constructor
	FrameSubpassSystem();
	/// destructor
	virtual ~FrameSubpassSystem();

	/// set call type
	void SetSubsystem(const Subsystem s);

	/// setup operation
	void Setup();
	/// run operation
	void Run(const IndexT frameIndex);
private:
	Subsystem call;
};


//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassSystem::SetSubsystem(const Subsystem s)
{
	this->call = s;
}

} // namespace Frame2