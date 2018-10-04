#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11TextRenderer
  
    Implements a simple text renderer for Direct3D11. This is only intended
    for outputting debug text, not for high-quality text rendering!

    FIXME: Need to handle Lost Device (ID3DXFont)
    
    (C) 2012 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
//------------------------------------------------------------------------------
#include "coregraphics/base/textrendererbase.h"
#include "FW1FontWrapper.h"

namespace Direct3D11
{
class D3D11TextRenderer : public Base::TextRendererBase
{
    __DeclareClass(D3D11TextRenderer);
    __DeclareSingleton(D3D11TextRenderer);
public:
    /// constructor
    D3D11TextRenderer();
    /// destructor
    virtual ~D3D11TextRenderer();

    /// open the device
    void Open();
    /// close the device
    void Close();
    /// draw the accumulated text
    void DrawTextElements();

private:


	IFW1Factory* fontFactory;
	IFW1FontWrapper* fontWrapper;
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
