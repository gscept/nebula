#pragma once
//------------------------------------------------------------------------------
/**
    @class Dynui::ImguiInputHandler
    
    This input handler passes input events to Imgui.
    
    @copyright
    (C) 2012-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "input/inputhandler.h"

namespace Dynui
{
class ImguiInputHandler : public Input::InputHandler
{
    __DeclareClass(ImguiInputHandler);
public:
    /// constructor
    ImguiInputHandler();
    /// destructor
    virtual ~ImguiInputHandler();

    /// capture input to this event handler
    virtual void BeginCapture() override;
    /// end input capturing to this event handler
    virtual void EndCapture() override;

    /// reset key inputs
    void OnBeginFrame() override;

protected:

    /// called when an input event should be processed
    virtual bool OnEvent(const Input::InputEvent& inputEvent) override;

private:
    Util::Array<Input::InputEvent> inputEvents;
};
} // namespace Dynui
