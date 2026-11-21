//------------------------------------------------------------------------------
//  @file editorlog.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/base/editorlog.h"

namespace Editor
{

__ImplementClass(Editor::EditorLog, 'ELOG', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
EditorLog::EditorLog() :
    maxLogSize(1024)
{
    this->timer.Start();
}

//------------------------------------------------------------------------------
/**
*/
EditorLog::~EditorLog()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorLog::Append(LogMessageType type, const Util::String& msg)
{
    LogEntry entry;
    entry.timestamp = this->Timestamp();
    entry.type = type;
    entry.msg = msg;

    this->critSect.Enter();
    this->entries.Append(entry);
    if (this->entries.Size() > this->maxLogSize)
        this->entries.EraseIndex(0);
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorLog::Clear()
{
    this->critSect.Enter();
    this->entries.Clear();
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorLog::Enter()
{
    this->critSect.Enter();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorLog::Leave()
{
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<LogEntry>&
EditorLog::GetEntries() const
{
    return this->entries;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
EditorLog::GetMaxLogSize() const
{
    return this->maxLogSize;
}

//------------------------------------------------------------------------------
/**
*/
const char*
EditorLog::Tag(LogMessageType type)
{
    switch (type)
    {
    case LogMessageType::N_INPUT:
        return ">>";
    case LogMessageType::N_WARNING:
        return "[WARNING]";
    case LogMessageType::N_ERROR:
        return "[ERROR]";
    case LogMessageType::N_EXCEPTION:
        return "[FATAL ERROR]";
    case LogMessageType::N_SYSTEM:
        return "[SYSTEM]";
    default:
        break;
    }
    return "[MESSAGE]";
}

//------------------------------------------------------------------------------
/**
    Current session time as hh:mm:ss
*/
Util::String
EditorLog::Timestamp() const
{
    const Timing::Time seconds = this->timer.GetTime();
    const int hour = (int)(seconds / 3600.0);
    const int minute = (int)((seconds - hour * 3600.0) / 60.0);
    const int second = (int)(seconds - hour * 3600.0 - minute * 60.0);

    Util::String timestamp;
    timestamp.Format("%02d:%02d:%02d", hour, minute, second);
    return timestamp;
}

} // namespace Editor
