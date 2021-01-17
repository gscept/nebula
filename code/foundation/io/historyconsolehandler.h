#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::HistoryConsoleHandler
    
    A console handler which stores the last N log messages in a
    Util::RingBuffer<String>.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/consolehandler.h"
#include "util/ringbuffer.h"

//------------------------------------------------------------------------------
namespace IO
{
class HistoryConsoleHandler : public ConsoleHandler
{
    __DeclareClass(HistoryConsoleHandler);
public:
    /// constructor
    HistoryConsoleHandler();

    /// set history size
    void SetHistorySize(SizeT numLines);
    /// get history size
    SizeT GetHistorySize() const;
    /// get accumulated log messages
    const Util::RingBuffer<Util::String>& GetHistory() const;

    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);

private:
    Util::RingBuffer<Util::String> history;
};

//------------------------------------------------------------------------------
/**
*/
inline void
HistoryConsoleHandler::SetHistorySize(SizeT numLines)
{
    this->history.SetCapacity(numLines);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
HistoryConsoleHandler::GetHistorySize() const
{
    return this->history.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::RingBuffer<Util::String>&
HistoryConsoleHandler::GetHistory() const
{
    return this->history;
}

} // namespace IO
//------------------------------------------------------------------------------
