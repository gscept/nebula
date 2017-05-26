//------------------------------------------------------------------------------
//  rendermodule.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "rendermodules/rendermodule.h"
#include "graphics/graphicsserver.h"

namespace RenderModules
{
__ImplementClass(RenderModules::RenderModule, 'RMDL', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
RenderModule::RenderModule() :
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
RenderModule::~RenderModule()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
RenderModule::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;

    // register with the graphics server (for per-frame callbacks)
    Graphics::GraphicsServer::Instance()->RegisterRenderModule(this);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderModule::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;

    // unregister from graphics server
    Graphics::GraphicsServer::Instance()->UnregisterRenderModule(this);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderModule::OnFrame()
{
    // empty, override in subclass if required
}
    
} // namespace Graphics
