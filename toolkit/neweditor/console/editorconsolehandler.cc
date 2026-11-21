//------------------------------------------------------------------------------
//  @file editorconsolehandler.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/console/editorconsolehandler.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "io/console.h"

namespace Editor
{

__ImplementClass(Editor::EditorConsoleHandler, 'ECHH', IO::ConsoleHandler);

//------------------------------------------------------------------------------
/**
*/
EditorConsoleHandler::EditorConsoleHandler() :
    core(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EditorConsoleHandler::~EditorConsoleHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Setup(Shell* core)
{
    n_assert(IO::Console::HasInstance());
    n_assert(core != nullptr);

    this->core = core;
    IO::Console::Instance()->AttachHandler(this);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Discard()
{
    if (this->core == nullptr)
        return;

    IO::Console::Instance()->RemoveHandler(this);
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Open()
{
    n_assert(this->core != nullptr);
    ConsoleHandler::Open();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Close()
{
    ConsoleHandler::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Print(const Util::String& msg)
{
    this->core->Log(LogMessageType::N_MESSAGE, msg);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Error(const Util::String& msg)
{
    this->core->Log(LogMessageType::N_ERROR, msg);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::Warning(const Util::String& msg)
{
    this->core->Log(LogMessageType::N_WARNING, msg);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorConsoleHandler::DebugOut(const Util::String& msg)
{
    this->core->Log(LogMessageType::N_SYSTEM, msg);
}

} // namespace Editor
