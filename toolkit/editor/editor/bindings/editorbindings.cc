//------------------------------------------------------------------------------
//  editorbindings.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

#include "pybind11/embed.h"
#include "scripting/python/conversion.h"

namespace py = pybind11;

/// @todo   There should be no more than one python module per binding file.
//PYBIND11_EMBEDDED_MODULE(edit, m)
//{
//    //// Begin macro
//    //m.def("begin_macro", &Edit::CommandManager::BeginMacro, "Begin a new macro. Bundles subsequent commands into one undo stack entry. Remember to call 'edit.end_macro()'!");
//    //
//    //// End macro
//    //m.def("end_macro", &Edit::CommandManager::EndMacro);
//    //
//    //// Undo last command
//    //m.def("undo", &Edit::CommandManager::Undo, "Undo last executed command.");
//    //
//    //// Redo
//    //m.def("redo", &Edit::CommandManager::Redo, "Redo last unexecuted command.");
//}

