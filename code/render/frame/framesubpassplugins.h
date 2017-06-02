#pragma once
//------------------------------------------------------------------------------
/**
	Executes RT plugins within a subpass
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "rendermodules/rt/rtpluginregistry.h"
namespace Frame
{
class FrameSubpassPlugins : public FrameOp
{
	__DeclareClass(FrameSubpassPlugins);
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
	/// run plugin pass
	void Run(const IndexT frameIndex);
private:
	Util::StringAtom pluginFilter;
	Ptr<RenderModules::RTPluginRegistry> pluginRegistry;
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