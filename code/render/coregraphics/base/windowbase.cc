//------------------------------------------------------------------------------
// windowbase.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "windowbase.h"

namespace Base
{
__ImplementClass(WindowBase, 'WNBS', Core::RefCounted);

IndexT WindowBase::uniqueWindowCounter = 0;
//------------------------------------------------------------------------------
/**
*/
WindowBase::WindowBase() :
	displayMode(0, 0, 1024, 768, CoreGraphics::PixelFormat::SRGBA8),
	antiAliasQuality(CoreGraphics::AntiAliasQuality::None),
	fullscreen(false),
	monitor(-1),
	modeSwitchEnabled(true),
	tripleBufferingEnabled(false),
	alwaysOnTop(false),
	windowTitle("Nebula3 Application Window"),
	iconName("NebulaIcon"),
	windowData(0),
	embedded(false),
	resizable(true),
	decorated(true),
	cursorVisible(true),
	cursorLocked(false),
	defaultRenderTexture(0),
	swapFrame(-1)
{
	this->windowId = uniqueWindowCounter++;
}

//------------------------------------------------------------------------------
/**
*/
WindowBase::~WindowBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::Open()
{
	n_assert(!this->defaultRenderTexture.isvalid());

	// create default render target (if not overriden by application
	this->defaultRenderTexture = CoreGraphics::RenderTexture::Create();
	this->defaultRenderTexture->SetIsWindowTexture(true);
	this->defaultRenderTexture->Setup();
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::Close()
{
	// release default render target
	if (this->defaultRenderTexture.isvalid())
	{
		this->defaultRenderTexture->Discard();
	}
	this->defaultRenderTexture = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::Reopen()
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::EnableCallbacks()
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::DisableCallbacks()
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::Resize(SizeT width, SizeT height)
{
	// set dimensions and reallocate the backbuffer target
	this->defaultRenderTexture->SetDimensions((float)width, (float)height);
	this->defaultRenderTexture->Resize();
}

//------------------------------------------------------------------------------
/**
*/
void
WindowBase::MakeCurrent()
{
	/// override in subclass
}


} // namespace Base