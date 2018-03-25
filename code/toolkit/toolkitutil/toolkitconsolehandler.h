#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TookitConsoleHandler

	Console handler for toolkit applications that will not abort in case of errors but 
	log the result of operations so that parent applications can handle the errors
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
#include "io/consolehandler.h"
#include "util/array.h"
#include "util/string.h"
#include "core/singleton.h"
#include "threading/threadid.h"
#include "threading/thread.h"
#include "io/xmlwriter.h"
#include "io/xmlreader.h"
//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class ToolkitConsoleHandler : public IO::ConsoleHandler
{
	__DeclareClass(ToolkitConsoleHandler);
	__DeclareInterfaceSingleton(ToolkitConsoleHandler);
public:
    /// constructor
	ToolkitConsoleHandler();
    /// destructor
	virtual ~ToolkitConsoleHandler();
      
    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);

	/// write output to console as well, not just store
	void SetConsoleOutputLevel(unsigned char logLevel);

	enum LogEntryLevel
	{
		LogDebug = 0x01,
		LogInfo = 0x02,
		LogWarning = 0x04,
		LogError   = 0x08,
		//
		LogAll	   = 0xff,
	};

	struct LogEntry
	{
		LogEntryLevel level;
		Util::String message;		
	};

	/// clear buffers
	void Clear();
	/// get errors
	Util::Array<Util::String> GetErrors();
	/// get errors
	Util::Array<Util::String> GetWarnings();
	/// get full console output
	const Util::Array<LogEntry> & GetLog();
	/// what kinds of messages occurred since last clear
	unsigned char GetLevels() const;

private:    
	///
	void Append(const LogEntry& entry);
	Threading::CriticalSection cs;
	Util::Dictionary<Threading::ThreadId,Util::Array<LogEntry>> log;
	Util::Dictionary<Threading::ThreadId, unsigned char> currentFlags;
	unsigned char logLevel;
	Ptr<IO::ConsoleHandler> systemConsole;
};

struct ToolLogEntry
{
	Util::String tool;
	Util::String source;
	unsigned char logLevels;
	Util::Array<ToolkitConsoleHandler::LogEntry> logs;
};
struct ToolLog
{
	Util::String asset;
	unsigned char logLevels;
	Util::Array<ToolLogEntry> logs;
	ToolLog(const Util::String & ass) :asset(ass), logLevels(0){}
	ToolLog() :logLevels(0){}
	void AddEntry(const Ptr<ToolkitUtil::ToolkitConsoleHandler> & console, const Util::String & tool, const Util::String & source);
	void ToString(const Ptr<IO::XmlWriter> & writer);
	static ToolLog FromString(const Ptr<IO::XmlReader> & reader);
};

//------------------------------------------------------------------------------
/**
*/
inline void
ToolkitUtil::ToolLog::AddEntry(const Ptr<ToolkitUtil::ToolkitConsoleHandler> & console, const Util::String & tool, const Util::String & source)
{
	this->logLevels |= console->GetLevels();
	this->logs.Append({ tool, source, console->GetLevels(), console->GetLog() });
}

///------------------------------------------------------------------------------
/**
*/
inline unsigned char
ToolkitConsoleHandler::GetLevels() const
{
	Threading::ThreadId id = Threading::Thread::GetMyThreadId();
	return this->currentFlags[id];
}

///------------------------------------------------------------------------------
/**
*/
inline void
ToolkitConsoleHandler::SetConsoleOutputLevel(unsigned char level)
{
	this->logLevel = level;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
