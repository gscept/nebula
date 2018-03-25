//------------------------------------------------------------------------------
//  toolkitconsolehandler.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitconsolehandler.h"
#include "threading/thread.h"
#include "io/memorystream.h"
#include "io/xmlwriter.h"
#include "io/xmlreader.h"
#ifdef WIN32
#include "io/win32/win32consolehandler.h"
#else
#include "io/posix/posixconsolehandler.h"
#endif

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ToolkitConsoleHandler, 'TCCH', IO::ConsoleHandler);
__ImplementInterfaceSingleton(ToolkitUtil::ToolkitConsoleHandler);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ToolkitConsoleHandler::ToolkitConsoleHandler() : logLevel(LogError)
{
	__ConstructInterfaceSingleton;
#ifdef WIN32
	Ptr<Win32::Win32ConsoleHandler> output = Win32::Win32ConsoleHandler::Create();
	this->systemConsole = output.cast<IO::ConsoleHandler>();
#else
	Ptr<Posix::PosixConsoleHandler> output = Posix::PosixConsoleHandler::Create();
	this->systemConsole = output.cast<IO::ConsoleHandler>();
#endif
}

//------------------------------------------------------------------------------
/**
*/
ToolkitConsoleHandler::~ToolkitConsoleHandler()
{
	__DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitConsoleHandler::Print(const String& str)
{

	this->Append({ LogInfo, str });	
	if (this->logLevel & LogInfo)
	{
		this->systemConsole->Print(str);		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitConsoleHandler::Error(const String& s)
{
	this->Append({ LogError, s });	
	if (this->logLevel & LogError)
	{
		this->systemConsole->Print(s);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitConsoleHandler::Warning(const String& s)
{
	this->Append({ LogWarning, s });	
	if (this->logLevel & LogWarning)
	{
		this->systemConsole->Print(s);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitConsoleHandler::DebugOut(const String& s)
{
	this->Append({ LogDebug, s });	
	if (this->logLevel & LogDebug)
	{
		this->systemConsole->Print(s);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitConsoleHandler::Clear()
{
	Threading::ThreadId id = Threading::Thread::GetMyThreadId();
	if (this->log.Contains(id))
	{
		this->currentFlags[id] = 0;
		this->log[id].Clear();
	}	
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
ToolkitConsoleHandler::GetErrors()
{
	Threading::ThreadId id = Threading::Thread::GetMyThreadId();
	Util::Array<Util::String> errors;
	for (Util::Array<LogEntry>::Iterator iter = this->log[id].Begin(); iter != this->log[id].End(); iter++)
	{
		if (iter->level == LogError)
		{
			errors.Append(iter->message);
		}
	}
	return errors;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
ToolkitConsoleHandler::GetWarnings()
{
	Util::Array<Util::String> warnings;
	Threading::ThreadId id = Threading::Thread::GetMyThreadId();
	for (Util::Array<LogEntry>::Iterator iter = this->log[id].Begin(); iter != this->log[id].End(); iter++)
	{
		if (iter->level == LogWarning)
		{
			warnings.Append(iter->message);
		}
	}
	return warnings;
}
//------------------------------------------------------------------------------
/**
*/
const Util::Array<ToolkitConsoleHandler::LogEntry> &
ToolkitConsoleHandler::GetLog()
{
	return this->log[Threading::Thread::GetMyThreadId()];
}

//------------------------------------------------------------------------------
/**
*/
void
ToolkitConsoleHandler::Append(const LogEntry& entry)
{
	this->cs.Enter();
	Threading::ThreadId id = Threading::Thread::GetMyThreadId();
	if (!this->log.Contains(id))
	{
		this->log.Add(id, Util::Array<LogEntry>());
		this->currentFlags.Add(id, entry.level);
	}	
	this->log[id].Append(entry);
	this->currentFlags[id] |= entry.level;
	this->cs.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
ToolLog::ToString(const Ptr<IO::XmlWriter> & writer)
{
	writer->BeginNode("Log");
	writer->SetString("asset", this->asset);
	writer->SetInt("levels", this->logLevels);	
	for (auto iter = this->logs.Begin(); iter != this->logs.End(); iter++)
	{
		writer->BeginNode("entry");
		writer->SetString("tool", iter->tool);
		writer->SetString("source", iter->source);
		writer->SetInt("levels", iter->logLevels);
		for (auto iiter = iter->logs.Begin(); iiter != iter->logs.End(); iiter++)
		{
			writer->BeginNode("entry");
			writer->SetInt("level", iiter->level);
			writer->SetString("entry", iiter->message);
			writer->EndNode();
		}
		writer->EndNode();
	}
	writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
ToolkitUtil::ToolLog
ToolLog::FromString(const Ptr<IO::XmlReader> & reader)
{ 
	ToolkitUtil::ToolLog log;
	/*
	Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
	stream->Open();
	stream->SetSize(str.Length() + 1);
	void * data = stream->Map();
	Memory::Copy(str.AsCharPtr(), data, str.Length() + 1);
	stream->Close();
	stream->SetAccessMode(IO::Stream::ReadAccess);
	Ptr<IO::XmlReader> reader = IO::XmlReader::Create();
	reader->SetStream(stream.cast<IO::Stream>());
	reader->Open();
	reader->SetToFirstChild("Log");
	*/
	log.asset = reader->GetString("asset");
	log.logLevels = reader->GetInt("levels");
	if (reader->SetToFirstChild("entry"))
	{
		do
		{
			ToolkitUtil::ToolLogEntry entry;
			entry.tool = reader->GetString("tool");
			entry.source = reader->GetString("source");
			entry.logLevels = reader->GetInt("levels");			
			if (reader->SetToFirstChild("entry"))
			{
				do
				{
					ToolkitUtil::ToolkitConsoleHandler::LogEntry logentry;
					logentry.level = (ToolkitUtil::ToolkitConsoleHandler::LogEntryLevel)reader->GetInt("level");
					logentry.message = reader->GetString("entry");
					entry.logs.Append(logentry);
				} 
				while (reader->SetToNextChild("entry"));				
			}
			log.logs.Append(entry);
		} 
		while (reader->SetToNextChild("entry"));		
	}	
	return log;
}

} // namespace ToolkitUtil
