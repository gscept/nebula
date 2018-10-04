//------------------------------------------------------------------------------
// frameserver.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "frameserver.h"
#include "framescriptloader.h"

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
Ptr<Frame::FrameScript>
FrameServer::LoadFrameScript(const Resources::ResourceName& name, const IO::URI& path)
{
	n_assert(!this->frameScripts.Contains(name));
	Ptr<Frame::FrameScript> script = FrameScriptLoader::LoadFrameScript(path);
	script->SetResourceName(name);
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