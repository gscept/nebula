//------------------------------------------------------------------------------
//  @file editorpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/core/editorpanel.h"
#include "neweditor/api/shell.h"

namespace Editor
{

__ImplementClass(Editor::EditorPanel, 'EPNL', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
EditorPanel::EditorPanel() :
    core(nullptr),
    open(true)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EditorPanel::~EditorPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
EditorPanel::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorPanel::Setup(Shell* core)
{
    if (core == nullptr)
        return false;

    this->core = core;
    return this->OnSetup();
}

//------------------------------------------------------------------------------
/**
    The window is opened by the core, so the flags and the focus check are the same for every panel.
*/
bool
EditorPanel::BeginWindow()
{
    if (!this->open)
        return false;

    return ImGui::Begin(this->name.AsCharPtr(), &this->open);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorPanel::EndWindow()
{
    ImGui::End();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorPanel::Draw()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorPanel::OnFocus(bool focused)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorPanel::OnLog(LogMessageType type, const Util::String& msg)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EditorPanel::Focus()
{
    ImGui::SetWindowFocus(this->name.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorPanel::IsOpen() const
{
    return this->open;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorPanel::SetOpen(bool open)
{
    this->open = open;
}

//------------------------------------------------------------------------------
/**
*/
Shell*
EditorPanel::GetShell() const
{
    return this->core;
}

//------------------------------------------------------------------------------
/**
    The status bar is view state rather than model state, so this is one of the few things a panel
    is allowed to do while it is drawing.
*/
void
EditorPanel::SetStatus(const Util::String& msg, float duration)
{
    if (this->core == nullptr)
        return;
    this->core->SetStatus(msg, duration);
}

//------------------------------------------------------------------------------
/**
    A panel that changed the model while it was drawing would change the arrays it is still walking,
    so the action is queued and the core runs it after the frame.
*/
void
EditorPanel::QueueAction(Util::Delegate<void()> action)
{
    if (this->core == nullptr || !action.IsValid())
        return;
    this->core->QueueAction(action);
}

//------------------------------------------------------------------------------
/**
    Only the console has a command entry, so the panels that draw nothing to type into answer with
    nothing.
*/
void
EditorPanel::SeedCommand(const Util::String& command)
{
    // empty
}

} // namespace Editor
