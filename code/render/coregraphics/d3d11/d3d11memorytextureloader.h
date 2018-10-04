#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11MemoryTextureLoader
           
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//-----------------------------------------------------------------------------

#include "resources/resourcestreampool.h"
#include "coregraphics/pixelformat.h"

namespace Direct3D11
{
class D3D11MemoryTextureLoader : public Resources::ResourceLoader
{
	__DeclareClass(D3D11MemoryTextureLoader);
public:		
	/// set image buffer
	void SetImageBuffer(const void* buffer, SizeT width, SizeT height, CoreGraphics::PixelFormat::Code format);		
	/// performs actual loading
	virtual bool OnLoadRequested();
private:
	ID3D11Texture2D* d3d11Texture;
};
}
