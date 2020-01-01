//------------------------------------------------------------------------------
//  consolehandler.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/consolehandler.h"

namespace IO
{
__ImplementClass(IO::ConsoleHandler, 'CONH', Core::RefCounted);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ConsoleHandler::ConsoleHandler() :
    isOpen(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ConsoleHandler::~ConsoleHandler()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Close()
{
    n_assert(this->IsOpen());
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Update()
{
    n_assert(this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Print(const String& /*s*/)
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Error(const String& /*s*/)
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Warning(const String& s)
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::Confirm(const String& s)
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
void
ConsoleHandler::DebugOut(const String& s)
{
    // empty, override in subclass as needed
}

//------------------------------------------------------------------------------
/**
*/
bool
ConsoleHandler::HasInput()
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
String
ConsoleHandler::GetInput()
{
    return "";
}

} // namespace IO
