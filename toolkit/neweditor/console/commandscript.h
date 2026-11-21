#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::CommandScript

    Commands read from a file and run through the editor command system, one command per line. A
    line can start with a frame number in brackets, which is the frame it runs on. A line without
    one, or with one that is not a number, runs on the first frame. A line that starts with a hash
    is a comment.

    The commands run in the order they appear in the file, so a line with a frame number waits for
    the lines before it even when its own frame has already passed.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "neweditor/base/editorlog.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class Shell;

//------------------------------------------------------------------------------
/**
    One command from the file, with the frame it runs on
*/
struct ScriptCommand
{
    IndexT frame;
    Util::String command;
};

class CommandScript : public Core::RefCounted
{
    __DeclareClass(CommandScript)
public:
    CommandScript();
    virtual ~CommandScript();

    /// read the file. The path can be a Nebula URI or a plain file path. Returns false if it could
    /// not be read
    bool Setup(Shell* core, const Util::String& path);
    /// clear the parsed commands
    void Discard();

    /// run the commands that are due this frame
    void Update();

    /// true when every command has been run
    bool IsFinished() const;

private:
    /// parse one line into a frame and a command
    ScriptCommand ParseLine(const Util::String& line) const;

    /// report something through the core, so it shows up in the console panel
    void Log(LogMessageType type, const Util::String& msg) const;

    Shell* core;
    Util::Array<ScriptCommand> commands;
    IndexT nextCommand;
    bool finished;
};

} // namespace Editor
