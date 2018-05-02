#pragma once
//------------------------------------------------------------------------------
/**
	@class CPlugin

	Hotswappable C++ component plugin.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "component/componentplugin.h"

struct cr_plugin;

namespace Game
{

class CPlugin : public ComponentPlugin
{
public:
	CPlugin();
	~CPlugin();

	bool LoadPlugin(const Util::String & path);

	bool UnloadPlugin() override;

	void OnBeginFrame(void* data) override;


private:
	cr_plugin* pluginContext;
};

} // namespace Game
