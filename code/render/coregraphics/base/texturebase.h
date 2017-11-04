#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::TextureBase
  
    Base class for texture construction, have a look at the the 
	allocators
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/   
#include "coregraphics/base/gpuresourcebase.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
    class Texture;
}

namespace Base
{
class TextureBase : public Base::GpuResourceBase
{
public:
    /// texture types
    enum Type
    {
        InvalidType,

		Texture1D,		//> a 1-dimensional texture
        Texture2D,      //> a 2-dimensional texture
        Texture3D,      //> a 3-dimensional texture
        TextureCube,    //> a cube texture

		Texture1DArray,	//> a 1-dimensional texture array, depth represents array size
		Texture2DArray,	//> a 2-dimensional texture array, depth represents array size
		Texture3DArray,	//> a 3-dimensional texture array, depth represents array size * size of layers in array
		TextureCubeArray //> a cube texture array, depth represents array size * 6
    };

    /// cube map face
    enum CubeFace
    {
        PosX = 0,
        NegX,
        PosY,
        NegY,
        PosZ,
        NegZ,
    };

    /// access info filled by Map methods
    class MapInfo
    {
    public:
        /// constructor
        MapInfo() : data(0), rowPitch(0), depthPitch(0) {};
        
        void* data;
        SizeT rowPitch;
        SizeT depthPitch;
		SizeT mipWidth;
		SizeT mipHeight;
    };

	struct Dimensions
	{
		SizeT width, height, depth;
	};

	
protected:
	friend class RenderTextureBase;
};


} // namespace Base
//------------------------------------------------------------------------------
