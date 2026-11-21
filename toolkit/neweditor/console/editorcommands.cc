//------------------------------------------------------------------------------
//  @file editorcommands.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/editorlog.h"
#include "core/cvar.h"

namespace Editor
{

__ImplementClass(Editor::EditorCommandSystem, 'ECMD', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
EditorCommandSystem::EditorCommandSystem() :
    core(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EditorCommandSystem::~EditorCommandSystem()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCommandSystem::Setup(Shell* core)
{
    this->core = core;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCommandSystem::Discard()
{
    this->commands.Clear();
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorCommandSystem::Register(
    const Util::String& name,
    const Util::String& description,
    Util::Delegate<void(const Util::Array<Util::String>& args)> func)
{
    n_assert(!name.IsEmpty());
    n_assert(func);

    for (IndexT i = 0; i < this->commands.Size(); i++)
    {
        if (this->commands[i].name == name)
        {
            this->commands[i].func = func;
            return;
        }
    }

    EditorCommand command;
    command.name = name;
    command.description = description;
    command.func = func;
    this->commands.Append(command);
}

//------------------------------------------------------------------------------
/**
    The words of a command line. A word that starts with a quote keeps its spaces, so a name with
    spaces can be given: 'select "Entity 1" "Entity 2" entity3'. A run of whitespace is one
    boundary, not an empty word, which is what the plain Tokenize() has always done. A quote that is
    never closed takes the rest of the line, so a stray quote is one word rather than half of one,
    and a quote that is not at the start of a word is part of it.
*/
Util::Array<Util::String>
TokenizeCommandLine(const Util::String& line)
{
    Util::Array<Util::String> tokens;

    const char* str = line.AsCharPtr();
    IndexT i = 0;
    while (str[i] != '\0')
    {
        while (str[i] == ' ' || str[i] == '\t')
            i++;
        if (str[i] == '\0')
            break;

        IndexT start = i;
        if (str[i] == '"')
        {
            start = ++i;
            while (str[i] != '\0' && str[i] != '"')
                i++;
        }
        else
        {
            while (str[i] != '\0' && str[i] != ' ' && str[i] != '\t')
                i++;
        }

        Util::String token;
        token.Set(&(str[start]), i - start);
        tokens.Append(token);

        if (str[i] == '"')
            i++;
    }

    return tokens;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCommandSystem::Execute(const Util::String& command)
{
    n_assert(!command.IsEmpty());

    Util::Array<Util::String> splits = TokenizeCommandLine(command);
    if (splits.IsEmpty())
        return false;

    Util::String verb = splits[0];
    verb.ToLower();

    for (IndexT i = 0; i < this->commands.Size(); i++)
    {
        EditorCommand& registered = this->commands[i];
        if (registered.name == verb)
        {
            Util::Array<Util::String> args;
            for (IndexT j = 1; j < splits.Size(); j++)
                args.Append(splits[j]);

            registered.func(args);
            return true;
        }
    }

    return this->EvaluateCVar(splits);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<EditorCommand>&
EditorCommandSystem::GetCommands() const
{
    return this->commands;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorCommandSystem::EvaluateCVar(const Util::Array<Util::String>& splits)
{
    const char* cVarName = splits[0].AsCharPtr();
    if (cVarName[0] == '>')
        cVarName = cVarName + 1;

    Core::CVar* cVar = Core::CVarGet(cVarName);
    if (cVar == nullptr)
    {
        this->core->Log(LogMessageType::N_SYSTEM, "Invalid command or console variable. Type 'help' for the commands");
        return false;
    }

    if (splits.Size() > 1)
    {
        Core::CVarParseWrite(cVar, splits[1].AsCharPtr());
        Core::CVarSetModified(cVar, true);

        Util::String msg;
        msg.Format("%s = %s", Core::CVarGetName(cVar), this->CVarValueAsString(cVar).AsCharPtr());
        this->core->Log(LogMessageType::N_SYSTEM, msg);
    }
    else
    {
        this->core->Log(LogMessageType::N_MESSAGE, this->CVarValueAsString(cVar));
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
EditorCommandSystem::CVarValueAsString(Core::CVar* cVar) const
{
    switch (Core::CVarGetType(cVar))
    {
    case Core::CVar_Int:
        return Util::String::FromInt(Core::CVarReadInt(cVar));
    case Core::CVar_Float:
        return Util::String::FromFloat(Core::CVarReadFloat(cVar));
    case Core::CVar_String:
        return Util::String(Core::CVarReadString(cVar));
    default:
        break;
    }
    return Util::String();
}

} // namespace Editor
