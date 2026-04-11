#pragma once
//------------------------------------------------------------------------------
/**
    @class Dynui::ImguiDisplayEventHandler

    Handles display events such that ImGui is made aware of them

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "coregraphics/displayeventhandler.h"

//------------------------------------------------------------------------------
namespace Dynui
{
class ImguiDisplayEventHandler : public CoreGraphics::DisplayEventHandler
{
    __DeclareClass(ImguiDisplayEventHandler);
public:
    /// called when an event happens
    virtual bool HandleEvent(const CoreGraphics::DisplayEvent& event);
};

} // namespace GLFW
//------------------------------------------------------------------------------
