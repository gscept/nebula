#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::ConsoleHandler

    Base class for all console handlers. Console handlers are attached to
    Nebula's central console object and are notified by the console
    object about output and deliver input to the console.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "util/array.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace IO
{
class ConsoleHandler : public Core::RefCounted
{
    __DeclareClass(ConsoleHandler);
public:
    /// constructor
    ConsoleHandler();
    /// destructor
    virtual ~ConsoleHandler();
    /// called by console when attached
    virtual void Open();
    /// called by console when removed
    virtual void Close();
    /// return true if currently open
    bool IsOpen() const;
    /// called by Console::Update()
    virtual void Update();
    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);
    /// called by console to display a confirmation message box
    virtual void Confirm(const Util::String& s);
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);
    /// return true if input is available
    virtual bool HasInput();
    /// read available input
    virtual Util::String GetInput();

private:
    bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
ConsoleHandler::IsOpen() const
{
    return this->isOpen;
}

} // namespace IO
//------------------------------------------------------------------------------
