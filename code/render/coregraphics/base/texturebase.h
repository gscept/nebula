#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::TextureBase
  
    Base class for texture construction, have a look at the the 
	allocators
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
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


	
protected:
	friend class RenderTextureBase;
};


} // namespace Base
//------------------------------------------------------------------------------
