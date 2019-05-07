#pragma once
//------------------------------------------------------------------------------
/**
	A subpass system operation executes a pre-defined subsystem, like lights, UI, text etc.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
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
	
	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		Subsystem call;
	};

	FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator);
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