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
	this->lock.Enter();
	this->event = event;
	this->lock.Leave();
	this->Flush();
}

} // namespace CoreGraphics
