//------------------------------------------------------------------------------
//  d3d11textrenderer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/d3d11/D3D11TextRenderer.h"
#include "coregraphics/renderdevice.h"
#include "coregraphics/displaydevice.h"
#include "threading/thread.h"
#include <cstdlib>
#include <string>

namespace Direct3D11
{
__ImplementClass(Direct3D11::D3D11TextRenderer, 'D1TR', Base::TextRendererBase);
__ImplementSingleton(Direct3D11::D3D11TextRenderer);

using namespace CoreGraphics;
using namespace Threading;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
D3D11TextRenderer::D3D11TextRenderer():
	fontFactory(0),
	fontWrapper(0)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
D3D11TextRenderer::~D3D11TextRenderer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
D3D11TextRenderer::Open()
{
    n_assert(!this->IsOpen());
	n_assert(0 == this->fontFactory);
	n_assert(0 == this->fontWrapper);
	

    // call parent
    Base::TextRendererBase::Open();

	// setup FW1 font factory and wrapper
	HRESULT hr = FW1CreateFactory(FW1_VERSION, &this->fontFactory);
	n_assert(SUCCEEDED(hr));
	hr = this->fontFactory->CreateFontWrapper(RenderDevice::Instance()->GetDirect3DDevice(), L"Arial", &this->fontWrapper);
	n_assert(SUCCEEDED(hr));

}

//------------------------------------------------------------------------------
/**
*/
void
D3D11TextRenderer::Close()
{
    n_assert(this->IsOpen());
	n_assert(0 != this->fontWrapper);
	n_assert(0 != this->fontFactory);


	this->fontWrapper->Release();
	this->fontFactory->Release();

	this->fontWrapper = 0;
	this->fontFactory = 0;

    // call base class
    Base::TextRendererBase::Close();
}

//------------------------------------------------------------------------------
/**
    Draw buffered text. This method is called once per frame.
*/
void
D3D11TextRenderer::DrawTextElements()
{
    n_assert(this->IsOpen());
	D3D11RenderDevice* renderDevice = D3D11RenderDevice::Instance();
	renderDevice->SetViewports();
	renderDevice->SetRenderTargets();
    const DisplayMode& displayMode = DisplayDevice::Instance()->GetDisplayMode();
       
    // draw text elements
    IndexT i;
    for (i = 0; i < this->textElements.Size(); i++)
    {
        const TextElement& curTextElm = this->textElements[i];
        const float4& color = curTextElm.GetColor();
		const float fontSize = curTextElm.GetSize();
		float2 position = curTextElm.GetPosition();

		// calculate screen-relative position from normalized space
		position.x() *= displayMode.GetWidth();
		position.y() *= displayMode.GetHeight();

		// unpack color to 8-bit integer
		uint red = (uint)color.x() * 255;
		uint green = (uint)color.y() * 255;
		uint blue = (uint)color.z() * 255;
		uint alpha = (uint)color.w() * 255;

		// merge color channels into 32 bit integer
		int rgb = ((alpha << 24) & 0xFF000000) | ((blue << 16) & 0x00FF0000) |((green << 8) & 0x0000FF00) | (red & 0x000000FF);

		DWORD size = MultiByteToWideChar(CP_ACP, 0, curTextElm.GetText().AsCharPtr(), -1, NULL, 0);
		wchar_t* wideChar = new wchar_t[size];
		MultiByteToWideChar(CP_UTF8, 0, curTextElm.GetText().AsCharPtr(), -1, wideChar, size);


		this->fontWrapper->DrawString(RenderDevice::Instance()->GetDirect3DDeviceContext(),
									  wideChar,
									  fontSize,
									  position.x(),
									  position.y(),
									  rgb,
									  i > 0 ? FW1_IMMEDIATECALL : 0);
		delete [] wideChar;
    }

    // delete the text elements of my own thread id, all other text elements
    // are from other threads and will be deleted through DeleteTextByThreadId()
    this->DeleteTextElementsByThreadId(Thread::GetMyThreadId());
    
}

} // namespace Direct3D11

