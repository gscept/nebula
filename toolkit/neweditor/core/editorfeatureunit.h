#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorFeatureUnit

    The composition root. It creates the core, registers the panels this editor has, and opens the
    core. A feature contributes at its own attach, so the editor is attached before the features it
    contributes to.

    The host gives the draw context and the keys. This part does not know which renderer owns the
    context, and it calls nothing on one.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "game/featureunit.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class EditorCore;
class EditorConsoleHandler;
class CommandScript;

//------------------------------------------------------------------------------
/**
*/
class EditorFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(EditorFeatureUnit)
public:
    EditorFeatureUnit();
    ~EditorFeatureUnit();

    void OnAttach() override;
    void OnActivate() override;
    void OnDeactivate() override;
    void OnFrame() override;

private:
    /// console command for play-in-editor
    void CommandPie(const Util::Array<Util::String>& args);
    /// console command for creating an entity in the active scene
    void CommandCreate(const Util::Array<Util::String>& args);
    /// console command for quitting the editor
    void CommandExit(const Util::Array<Util::String>& args);

    /// the value that follows a flag on the command line
    Util::String FlagValue(const Util::String& name) const;

    Ptr<EditorCore> core;
    Ptr<EditorConsoleHandler> consoleHandler;
    Ptr<CommandScript> script;
};

} // namespace Editor
//------------------------------------------------------------------------------
