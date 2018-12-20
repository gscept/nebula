#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderModules::RenderModule

    A RenderModule wraps a specific, optional, rendering functionality 
    into a simple object which only requires a simple setup.
    For instance, setting up the debug render functionality in an application
    looks like this:

    this->debugRenderModule = DebugRenderModule::Create();
    this->debugRenderModule->Setup();

    This will setup the required environment on the main-thread and
    render-thread side to implement debug rendering.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace RenderModules
{
class RenderModule : public Core::RefCounted
{
    __DeclareClass(RenderModule);
public:
    /// constructor
    RenderModule();
    /// destructor
    virtual ~RenderModule();

    /// setup the render module
    virtual void Setup();
    /// discard the render module
    virtual void Discard();
    /// return true if the render module has been setup
    bool IsValid() const;
    /// called per-frame by Graphics::GraphicsServer
    virtual void OnFrame();

private:
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
RenderModule::IsValid() const
{
    return this->isValid;
}

} // namespace RenderModules
//------------------------------------------------------------------------------
