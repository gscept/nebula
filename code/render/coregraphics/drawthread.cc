//------------------------------------------------------------------------------
//  drawthread.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "drawthread.h"
namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
*/
DrawThread::DrawThread()
    : event(nullptr)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
DrawThread::EmitWakeupSignal()
{
    this->signalEvent.Signal();
}

//------------------------------------------------------------------------------
/**
*/
void 
DrawThread::Flush()
{
    this->EmitWakeupSignal();
}

//------------------------------------------------------------------------------
/**
*/
void 
DrawThread::Signal(Threading::Event* event)
{
    SyncCommand cmd;
    cmd.event = event;
    this->Push(cmd);

    // add an extra flush
    this->Flush();
}

} // namespace CoreGraphics
