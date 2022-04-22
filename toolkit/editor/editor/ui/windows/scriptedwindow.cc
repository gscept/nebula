//------------------------------------------------------------------------------
//  scriptedwindow.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scriptedwindow.h"
#include "pybind11/pybind11.h"

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
        n_delete(this->script);
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
    catch (const pybind11::error_already_set& error)
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
        this->script = n_new(pybind11::object);
    }

    try
    {
        *(this->script) = pybind11::module::import(module.AsCharPtr());
    }
    catch (const pybind11::error_already_set& error)
    {
        n_warning(error.what());
        return false;
    }

    this->modulePath = module;
    return true;
}

}
