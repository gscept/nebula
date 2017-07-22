#pragma once

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
