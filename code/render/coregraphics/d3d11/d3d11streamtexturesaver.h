#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11StreamTextureSaver
    
    D3D11 implementation of StreamTextureSaver.
    
    (C) 2011-2013 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/streamtexturesaverbase.h"


namespace Direct3D11
{
class D3D11StreamTextureSaver : public Base::StreamTextureSaverBase
{
    __DeclareClass(D3D11StreamTextureSaver);


public:
    /// called by resource when a save is requested
    virtual bool OnSave();

private:
	/// unpacks DXT1 compressed image, width is each row width in bytes
	void DecompressDXT1(uint x, uint y, uchar* source, uint* destination, uint width);
	/// unpacks DXT3 compressed image, width is each row width in bytes
	void DecompressDXT3(uint x, uint y, uchar* source, uint* destination, uint width);
	/// unpacks DXT5 compressed image, width is each row width in bytes
	void DecompressDXT5(uint x, uint y, uchar* source, uint* destination, uint width);

	/// converts RG texture to RGBA unsigned byte
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
    