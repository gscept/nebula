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
#include "util/ringbuffer.h"
#include "io/textwriter.h"

namespace Dynui
{
class ImguiConsole : public Core::RefCounted
{
	__DeclareClass(ImguiConsole);
	__DeclareInterfaceSingleton(ImguiConsole);

public:
	/**
		These are all the types of messages that can be printed in the console
		Depending on the type of the message, they are sorted into different categories by appending a type prefix to the message
	*/
	enum LogMessageType
	{
		///Just plain white text. Adds [Message] prefix to the log message.
		N_MESSAGE = 0,
		///User Input text. Appends nothing to the message.
		N_INPUT = 1,
		///Warning text. Adds [Warning] to the message.
		N_WARNING = 2,
		///Error message. Adds [Error] to the message.
		N_ERROR = 3,
		///Exception. Adds [FATAL ERROR] to the messsage. These are only used when the application encounters an assertion and needs to abort
		N_EXCEPTION = 4,
		///default messages from system. Adds [SYSTEM] to the message
		N_SYSTEM = 5
	};

	struct LogEntry
	{
		///Using a string for timestamp, so that we can just show it right away
		// Util::String timestamp;
		///Message type. This will be used for setting the color of the message
		LogMessageType type;
		///Message string.
		Util::String msg;
	};

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
	void AppendToLog(const LogEntry& msg);


	//Util::Dictionary<Util::String, Ptr<Scripting::Command>> commands;
	Util::Array<Util::String> previousCommands;
	int previousCommandIndex;

private:
	const char * LogEntryTypeAsCharPtr(const LogMessageType & type) const;

	bool scrollToBottom;

	/// run script command
	void Execute(const Util::String& command);

	char command[65535];
	Util::RingBuffer<LogEntry> consoleBuffer;	
	IndexT selectedSuggestion;
	Ptr<Scripting::ScriptServer> scriptServer;
	bool moveScroll;
	bool visible;
    
    Ptr<IO::TextWriter> persistentHistory;
};
} // namespace Dynui