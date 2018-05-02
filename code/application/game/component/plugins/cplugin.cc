//------------------------------------------------------------------------------
//  cplugin.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "cplugin.h"

#define CR_HOST CR_UNSAFE // try to best manage static states
#include "crh/cr.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
CPlugin::CPlugin()
{
	this->pluginContext = new cr_plugin();
}

//------------------------------------------------------------------------------
/**
*/
CPlugin::~CPlugin()
{
	delete this->pluginContext;
}

//------------------------------------------------------------------------------
/**
*/
bool
CPlugin::LoadPlugin(const Util::String& path)
{
	// the full path to the live-reloadable application
	return cr_plugin_load(*this->pluginContext, path.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
bool
CPlugin::UnloadPlugin()
{
	cr_plugin_close(*this->pluginContext);
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
CPlugin::OnBeginFrame(void* data)
{
	this->pluginContext->userdata = data;
	cr_plugin_update(*this->pluginContext);
}



} // namespace Game
