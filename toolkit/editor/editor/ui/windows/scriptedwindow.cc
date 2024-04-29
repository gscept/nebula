//------------------------------------------------------------------------------
//  scriptedwindow.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scriptedwindow.h"
#include "nanobind/nanobind.h"
#include "io/ioserver.h"

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
    {
        delete this->script;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptedWindow::Run(SaveMode save)
{
    if (this->script->is_none())
    {
        return;
    }
    ImGui::Begin(this->name.AsCharPtr());
    try
    {
        this->script->attr("draw")();        
    }
    catch (const nanobind::python_error& error)
    {
        n_warning(error.what());
    }
    ImGui::End();
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

    if (!nanobind::hasattr(*this->script, "draw"))
    {
        n_warning("ScriptedWindow: %s has no draw function", module.AsCharPtr());
        return false;
    }
    this->modulePath = module;
    if (nanobind::hasattr(*this->script, "category"))
    {
        nanobind::object catObj = nanobind::getattr(*this->script, "category");
        auto strob = nanobind::str(catObj);
        this->category = strob.c_str();
    }
    return true;
}

}
