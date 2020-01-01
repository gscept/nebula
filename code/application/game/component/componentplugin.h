#pragma once
//------------------------------------------------------------------------------
/**
	@class ComponentPlugin

	An interface for component functionality.

	Plugins can be ex. lua scripts or reloadable shared libraries.

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"

namespace Game
{

class ComponentPlugin
{
public:
	ComponentPlugin();
	~ComponentPlugin();
	
	/// Loads a plugin from path. Overload in subclass.
	virtual bool LoadPlugin(const Util::String& path);
	/// Unloads plugin. Overload in subclass.
	virtual bool UnloadPlugin();

	/// called on begin of frame. Overload in subclass.
	virtual void OnBeginFrame(void* data);
	/*
	/// called when an instance is activated.
	void OnActivate();
	/// called when an instance is deactivated.
	void OnDeactivate();
	/// called after attributes are loaded
	void OnLoad();
	/// called after OnLoad when the complete world exist
	void OnStart();
	/// called before attributes are saved back to database
	void OnSave();

	/// called before rendering happens
	void OnRender();
	/// called when game debug visualization is on
	void OnRenderDebug();
	/// called when entity looses activity
	void OnLoseActivity();
	/// called when entity gains activity
	void OnGainActivity();
	*/

protected:

};

} // namespace Game
