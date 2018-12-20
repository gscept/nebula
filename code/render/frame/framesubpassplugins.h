#pragma once
//------------------------------------------------------------------------------
/**
	Executes RT plugins within a subpass
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"

namespace Frame
{
class FrameSubpassPlugins : public FrameOp
{
public:
	/// constructor
	FrameSubpassPlugins();
	/// destructor
	virtual ~FrameSubpassPlugins();

	/// set filter to use for plugins
	void SetPluginFilter(const Util::StringAtom& str);

	/// setup plugin pass
	void Setup();
	/// discard operation
	void Discard();

	struct CompiledImpl : public FrameOp::Compiled
	{
		void Run(const IndexT frameIndex);

		Util::StringAtom pluginFilter;		
	};

	FrameOp::Compiled* AllocCompiled(Memory::ChunkAllocator<BIG_CHUNK>& allocator);
private:
	Util::StringAtom pluginFilter;	
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameSubpassPlugins::SetPluginFilter(const Util::StringAtom& str)
{
	this->pluginFilter = str;
}

} // namespace Frame2