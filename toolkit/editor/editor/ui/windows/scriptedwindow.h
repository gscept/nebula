#pragma once
//------------------------------------------------------------------------------
/**
    A scripted window

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "io/uri.h"

namespace pybind11
{
class object;
}

namespace Presentation
{

class ScriptedWindow : public BaseWindow
{
    __DeclareClass(ScriptedWindow)
public:
    ScriptedWindow();
    ~ScriptedWindow();

    void Run();

    /// Set the python module name.
    bool LoadModule(Util::String const& modulePath);

private:
    // loaded python module
    pybind11::object* script;
    Util::String modulePath;
};

} // namespace Interface
