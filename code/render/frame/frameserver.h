#pragma once
//------------------------------------------------------------------------------
/**
	The frame server is responsible for keeping track of frame scripts.

	A frame script attachment using the name __WINDOW__ will, instead of 
	creating a texture, simply fetch the texture from the frame server.
	Use this mechanic to switch which window to render to.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "framescript.h"
#include "io/uri.h"
namespace Frame
{
class FrameServer : public Core::RefCounted
{
	__DeclareClass(FrameServer);
	__DeclareSingleton(FrameServer);
public:
	/// constructor
	FrameServer();
	/// destructor
	virtual ~FrameServer();

	/// open server
	void Open();
	/// returns true if open
	bool IsOpen() const;
	/// close server, discards all remaining scripts
	void Close();
    /// propagate resize to all in all scripts
    void OnWindowResize();

	/// load frame script and save with name
	Ptr<FrameScript> LoadFrameScript(const Resources::ResourceName& name, const IO::URI& path);
	/// get script by name
	const Ptr<FrameScript>& GetFrameScript(const Resources::ResourceName& name);
	/// unload frame script
	void UnloadFrameScript(const Resources::ResourceName& name);

	/// set texture to be used for future loads of frame scripts
	void SetWindowTexture(const CoreGraphics::TextureId& tex);
	/// get window texture
	const CoreGraphics::TextureId& GetWindowTexture() const;

	/// update frame server when window resized
	void OnWindowResized(const CoreGraphics::WindowId window);
private:
	Util::Dictionary<Resources::ResourceName, Ptr<FrameScript>> frameScripts;
	CoreGraphics::TextureId windowTexture;
	bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameServer::SetWindowTexture(const CoreGraphics::TextureId& tex)
{
	this->windowTexture = tex;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::TextureId&
FrameServer::GetWindowTexture() const
{
	return this->windowTexture;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FrameServer::OnWindowResized(const CoreGraphics::WindowId window)
{
	IndexT i;
	for (i = 0; i < this->frameScripts.Size(); i++)
	{
		if (this->frameScripts.ValueAtIndex(i)->window == window)
			this->frameScripts.ValueAtIndex(i)->OnWindowResized();
	}
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Frame::FrameScript>&
FrameServer::GetFrameScript(const Util::StringAtom& name)
{
	return this->frameScripts[name];
}

//------------------------------------------------------------------------------
/**
*/
inline bool
FrameServer::IsOpen() const
{
	return this->isOpen;
}

} // namespace Frame2