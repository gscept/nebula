#pragma once
//------------------------------------------------------------------------------
/**
    Presentation::Console

    Console window wrapper

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "graphics/view.h"
#include "coregraphics/graphicsdevice.h"
#include "dynui/console/imguiconsole.h"
#include "dynui/console/imguiconsolehandler.h"

namespace Presentation
{

class Console : public BaseWindow
{
    __DeclareClass(Console)
public:
    Console();
    ~Console();

    void Run();
    void Update();

private:
    Ptr<Dynui::ImguiConsole> console;
    Ptr<Dynui::ImguiConsoleHandler> consoleHandler;

};

__RegisterClass(Console);

} // namespace Presentation