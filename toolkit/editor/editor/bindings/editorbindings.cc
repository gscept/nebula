//------------------------------------------------------------------------------
//  editorbindings.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editor/commandmanager.h"
#include "editor/editor.h"
#include "scripting/python/conversion.h"
#include "scripting/scriptserver.h"
#include "editor/ui/windowserver.h"

namespace py = nanobind;

extern "C" PyObject* PyInit_editor();

namespace
{

void
RegisterEditorPythonModule()
{
    if (!Py_IsInitialized())
    {
        PyImport_AppendInittab("editor", PyInit_editor);
        return;
    }

    PyGILState_STATE gilState = PyGILState_Ensure();

    PyObject* modules = PyImport_GetModuleDict();
    if (modules != nullptr && PyDict_GetItemString(modules, "editor") != nullptr)
    {
        PyGILState_Release(gilState);
        return;
    }

    PyObject* module = PyInit_editor();
    if (module == nullptr)
    {
        PyErr_Print();
        PyGILState_Release(gilState);
        return;
    }

    if (modules == nullptr || PyDict_SetItemString(modules, "editor", module) != 0)
    {
        PyErr_Print();
        Py_DECREF(module);
        PyGILState_Release(gilState);
        return;
    }

    Py_DECREF(module);
    PyGILState_Release(gilState);
}

}

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

    // Explicitly trigger a hot-reload of the editor feature module.
    // The reload is deferred to the next frame boundary and rejected if
    // play-in-editor is currently active.
    m.def("reload_editor_module", []() { Editor::RequestModuleReload("editorfeaturemodule", "editorfeaturemodule"); });

    // Generic module reload entry point. If build_target is omitted,
    // module_name is used for both build and reload.
    m.def(
        "reload_module",
        [](const char* moduleName, const char* buildTarget)
        {
            const Util::String module = moduleName != nullptr ? moduleName : "";
            const Util::String target = buildTarget != nullptr ? buildTarget : "";
            Editor::RequestModuleReload(module, target);
        },
        py::arg("module_name"),
        py::arg("build_target") = ""
    );
}

namespace Scripting
{
void RegisterEditorBinds()
{
    RegisterEditorPythonModule();
}
}