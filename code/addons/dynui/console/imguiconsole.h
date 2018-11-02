#pragma once
//------------------------------------------------------------------------------
/**
	@class Dynui::ImguiConsole
	
	The ImGui console uses ImGui to produce a live interactive console.
	
	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "scripting/scriptserver.h"
#include "scripting/command.h"
#include "util/ringbuffer.h"

namespace Dynui
{
class ImguiConsole : public Core::RefCounted
{
	__DeclareClass(ImguiConsole);
	__DeclareInterfaceSingleton(ImguiConsole);

public:
	/// constructor
	ImguiConsole();
	/// destructor
	virtual ~ImguiConsole();

	/// setup console, the ImguiAddon must be setup prior to this
	void Setup();
	/// discard console
	void Discard();

	/// render, call this per-frame
	void Render();
	
	/// add line to the log
	void AppendToLog(const Util::String & msg);

	Util::Dictionary<Util::String, Ptr<Scripting::Command>> commands;
	Util::Array<Util::String> previousCommands;
	int previousCommandIndex;

private:
	/// run script command
	void Execute(const Util::String& command);

	char command[65535];
	Util::RingBuffer<Util::String> consoleBuffer;	
	IndexT selectedSuggestion;
	Ptr<Scripting::ScriptServer> scriptServer;
	bool moveScroll;
	bool visible;
};
} // namespace Dynui