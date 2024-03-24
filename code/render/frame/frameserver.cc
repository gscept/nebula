//------------------------------------------------------------------------------
// frameserver.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "frameserver.h"
#include "framescriptloader.h"
#include "frameplugin.h"

namespace Frame
{

__ImplementClass(Frame::FrameServer, 'FRSR', Core::RefCounted);
__ImplementSingleton(Frame::FrameServer);
//------------------------------------------------------------------------------
/**
*/
FrameServer::FrameServer() :
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
FrameServer::~FrameServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameServer::Open()
{
    n_assert(!this->isOpen);
    this->isOpen = true;

    Frame::InitPluginTable();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameServer::Close()
{
    n_assert(this->isOpen);
    n_assert(this->frameScripts.IsEmpty());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameServer::OnWindowResize()
{
    for (IndexT i = 0; i < this->frameScripts.Size(); ++i)
    {
        this->frameScripts.ValueAtIndex(i)->OnWindowResized();
    }
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Frame::FrameScript>
FrameServer::LoadFrameScript(const Resources::ResourceName& name, const IO::URI& path, const CoreGraphics::WindowId window)
{
    n_assert(!this->frameScripts.Contains(name));
    Ptr<Frame::FrameScript> script = FrameScriptLoader::LoadFrameScript(path);
    script->SetResourceName(name);
    script->Setup();
    this->frameScripts.Add(name, script);
    return script;
}

//------------------------------------------------------------------------------
/**
*/
void
FrameServer::UnloadFrameScript(const Resources::ResourceName& name)
{
    n_assert(this->frameScripts.Contains(name));
    this->frameScripts[name]->Cleanup();
    this->frameScripts.Erase(name);
}

} // namespace Frame2
