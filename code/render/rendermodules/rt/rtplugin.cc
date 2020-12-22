//------------------------------------------------------------------------------
//  rtplugin.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "rendermodules/rt/rtplugin.h"

namespace RenderModules
{
__ImplementClass(RenderModules::RTPlugin, 'RTPG', Core::RefCounted);

using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
RTPlugin::RTPlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RTPlugin::~RTPlugin()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnRegister()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnUnregister()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnStageCreated(const Ptr<Stage>& stage)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnDiscardStage(const Ptr<Stage>& stage)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnViewCreated(const Ptr<View>& view)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnDiscardView(const Ptr<View>& view)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnAttachEntity(const Ptr<GraphicsEntity>& entity)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnRemoveEntity(const Ptr<GraphicsEntity>& entity)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnUpdateBefore(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnUpdateAfter(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnRenderBefore(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnRenderAfter(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnRender(const Util::StringAtom& filter)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnFrameBefore(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnFrameAfter(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
RTPlugin::OnRenderFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
RTPlugin::OnRenderWithoutView(IndexT frameId, Timing::Time time)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
RTPlugin::OnWindowResized(IndexT windowId, SizeT width, SizeT height)
{
    // empty
}


} // namespace RenderModules