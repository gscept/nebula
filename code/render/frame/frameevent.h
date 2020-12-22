#pragma once
//------------------------------------------------------------------------------
/**
    A frame event is a wait-signal-reset type event which can be used to wait for executions
    within a frame script.
    
    (C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "coregraphics/event.h"
namespace Frame
{
class FrameEvent : public FrameOp
{
public:

    enum Action
    {
        Set,
        Reset,
        Wait
    };

    /// constructor
    FrameEvent();
    /// destructor
    virtual ~FrameEvent();

    /// discard operation
    void Discard();
    /// run operation
    void Run(const IndexT frameIndex);

    Util::Array<Action> actions;
    CoreGraphics::EventId event;
    CoreGraphics::BarrierStage dependency;
    CoreGraphics::QueueType queueType; 
    CoreGraphics::CommandBufferUsage commandBufferType;
};

} // namespace Frame2