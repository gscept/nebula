//------------------------------------------------------------------------------
//  componentplugin.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "componentplugin.h"
namespace Game
{

//------------------------------------------------------------------------------
/**
*/
ComponentPlugin::ComponentPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ComponentPlugin::~ComponentPlugin()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
bool
ComponentPlugin::LoadPlugin(const Util::String & path)
{
	// empty
	return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
ComponentPlugin::UnloadPlugin()
{
	// empty
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentPlugin::OnBeginFrame(void * data)
{
	// empty
}

} // namespace Game
