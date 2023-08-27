//------------------------------------------------------------------------------
//  scriptedwindow.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scriptedwindow.h"
#include "nanobind/nanobind.h"

namespace Presentation
{
__ImplementClass(Presentation::ScriptedWindow, 'ScWn', Presentation::BaseWindow)

//------------------------------------------------------------------------------
/**
*/
ScriptedWindow::ScriptedWindow() :
    script(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ScriptedWindow::~ScriptedWindow()
{
    if (this->script != nullptr)
        delete this->script;
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptedWindow::Run()
{
    if (this->script->is_none())
        return;

    try
    {
        ImGui::Begin(this->name.AsCharPtr());
        this->script->attr("draw")();
        ImGui::End();
    }
    catch (const nanobind::python_error& error)
    {
        n_warning(error.what());
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ScriptedWindow::LoadModule(Util::String const& module)
{
    if (this->script == nullptr)
    {
        this->script = new nanobind::object;
    }

    try
    {
        *(this->script) = nanobind::module_::import_(module.AsCharPtr());
    }
    catch (const nanobind::python_error& error)
    {
        n_warning(error.what());
        return false;
    }

    this->modulePath = module;
    return true;
}

}
