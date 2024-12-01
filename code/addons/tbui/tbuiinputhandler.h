#pragma once
//------------------------------------------------------------------------------
/**
    @class TBUI::TBUIInputHandler
    
    This input handler passes input events to Turbobadger.
    
    @copyright
    (C) 2012-2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "input/inputhandler.h"

namespace TBUI
{
class TBUIInputHandler : public Input::InputHandler
{
    __DeclareClass(TBUIInputHandler);
public:
    /// constructor
    TBUIInputHandler();
    /// destructor
    virtual ~TBUIInputHandler();

    /// capture input to this event handler
    virtual void BeginCapture();
    /// end input capturing to this event handler
    virtual void EndCapture();

    /// reset key inputs
    void OnBeginFrame() override;

protected:

    /// called when an input event should be processed
    virtual bool OnEvent(const Input::InputEvent& inputEvent);

private:
    Util::Array<Input::InputEvent> inputEvents;
};
} // namespace TBUI