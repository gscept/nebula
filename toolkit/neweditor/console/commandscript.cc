//------------------------------------------------------------------------------
//  @file commandscript.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/console/commandscript.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "appgame/gameapplication.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/textreader.h"
#include "io/uri.h"

namespace Editor
{

__ImplementClass(Editor::CommandScript, 'CSCR', Core::RefCounted);

// A line without a frame prefix runs on the first frame
static const IndexT DefaultFrame = 1;

//------------------------------------------------------------------------------
/**
*/
CommandScript::CommandScript() :
    core(nullptr),
    nextCommand(0),
    finished(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
CommandScript::~CommandScript()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The path can be a Nebula URI, or a plain file path, which is read from disk.
*/
bool
CommandScript::Setup(Shell* core, const Util::String& path)
{
    this->core = core;
    this->commands.Clear();
    this->nextCommand = 0;
    this->finished = false;

    IO::URI uri(path);
    if (uri.Scheme().IsEmpty())
        uri = IO::URI(Util::String("file://") + path);

    Ptr<IO::Stream> stream = IO::CreateStream(uri);
    if (!stream.isvalid())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("Could not read the command file ") + path);
        return false;
    }

    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("Could not open the command file ") + path);
        return false;
    }

    Ptr<IO::TextReader> reader = IO::TextReader::Create();
    reader->SetStream(stream);
    if (!reader->Open())
    {
        this->Log(LogMessageType::N_WARNING, Util::String("Could not read the command file ") + path);
        reader->Close();
        stream->Close();
        return false;
    }

    Util::Array<Util::String> lines = reader->ReadAllLines();
    reader->Close();

    for (IndexT i = 0; i < lines.Size(); i++)
    {
        Util::String line = lines[i];
        line.Trim(" \t\r");
        if (line.IsEmpty())
            continue;

        // A line that starts with a hash is a comment, which is what lets a command file say what
        // it is
        // checking without the command system trying to run it
        if (line.BeginsWithString("#"))
            continue;

        const ScriptCommand entry = this->ParseLine(line);
        if (!entry.command.IsEmpty())
            this->commands.Append(entry);
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
CommandScript::Discard()
{
    this->commands.Clear();
    this->nextCommand = 0;
    this->finished = false;
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
CommandScript::Update()
{
    if (this->core == nullptr)
        return;

    const IndexT frame = App::GameApplication::FrameIndex;

    while (this->nextCommand < this->commands.Size())
    {
        const ScriptCommand& entry = this->commands[this->nextCommand];
        if (entry.frame > frame)
            break;

        this->core->Log(LogMessageType::N_INPUT, entry.command);
        this->core->ExecuteCommand(entry.command);
        this->nextCommand++;

        // A command can ask the application to quit, and the commands after that should not run
        if (App::GameApplication::Instance()->IsQuitRequested())
            break;
    }

    if (this->IsFinished() && !this->finished)
    {
        this->finished = true;
        this->Log(LogMessageType::N_SYSTEM, "Command file finished");
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
CommandScript::IsFinished() const
{
    return this->nextCommand >= this->commands.Size();
}

//------------------------------------------------------------------------------
/**
    A line that starts with a bracket is a frame number and the command that runs on that frame. A
    line that does not is the command on its own, and runs on the first frame.
*/
ScriptCommand
CommandScript::ParseLine(const Util::String& line) const
{
    ScriptCommand entry;
    entry.frame = DefaultFrame;

    if (!line.BeginsWithString("["))
    {
        entry.command = line;
        return entry;
    }

    const IndexT end = line.FindCharIndex(']');
    if (end == InvalidIndex)
    {
        this->Log(LogMessageType::N_WARNING, Util::String("Command file line '") + line + "' has no closing bracket");
        entry.command = line;
        return entry;
    }

    const Util::String frame = line.ExtractRange(1, end - 1);
    if (!frame.IsValidInt() || frame.AsInt() < 1)
        this->Log(LogMessageType::N_WARNING, Util::String("Command file line '") + line + "' has no frame number");
    else
        entry.frame = frame.AsInt();

    Util::String command = line.ExtractToEnd(end + 1);
    command.Trim(" \t");
    entry.command = command;

    return entry;
}

//------------------------------------------------------------------------------
/**
    Everything the script reports goes through the core, so it ends up in the console panel
*/
void
CommandScript::Log(LogMessageType type, const Util::String& msg) const
{
    if (this->core != nullptr)
        this->core->Log(type, msg);
    else
        n_warning("%s\n", msg.AsCharPtr());
}

} // namespace Editor
