//------------------------------------------------------------------------------
//  editorbindings.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editor/commandmanager.h"
#include "scripting/python/conversion.h"
#include "scripting/scriptserver.h"
#include "editor/ui/windowserver.h"

namespace py = nanobind;

/// @todo   There should be no more than one python module per binding file.
NB_MODULE(editor, m)
{
    // Begin macro
    m.def("begin_macro", &Edit::CommandManager::BeginMacro, "Begin a new macro. Bundles subsequent commands into one undo stack entry. Remember to call 'edit.end_macro()'!");
    
    // End macro
    m.def("end_macro", &Edit::CommandManager::EndMacro);
    
    // Undo last command
    m.def("undo", &Edit::CommandManager::Undo, "Undo last executed command.");
    
    // Redo
    m.def("redo", &Edit::CommandManager::Redo, "Redo last unexecuted command.");

    m.def("register_script_window", [](const char* scriptfile, const char* label)
    {
        n_assert(Presentation::WindowServer::HasInstance());
        Presentation::WindowServer::Instance()->RegisterWindowScript(scriptfile, label);
    });
}

namespace Scripting
{
void RegisterEditorBinds()
{
    Scripting::ScriptServer::RegisterModuleInit([]()
    {
        PyImport_AppendInittab("editor", PyInit_editor);
    });
}
}