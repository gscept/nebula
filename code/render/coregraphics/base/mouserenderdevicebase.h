#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::MouseRenderDeviceBase
  
    Renders one (or more, depending on platform) mouse cursors.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/mousepointer.h"
#include "coregraphics/texture.h"

//------------------------------------------------------------------------------
namespace Base
{
class MouseRenderDeviceBase : public Core::RefCounted
{
    __DeclareClass(MouseRenderDeviceBase);
    __DeclareSingleton(MouseRenderDeviceBase);
public:
    /// constructor
    MouseRenderDeviceBase();
    /// destructor
    virtual ~MouseRenderDeviceBase();
    
    /// open the mouse renderer
    void Setup();
    /// close the mouse renderer
    void Discard();
    /// return true if the mouse renderer is valid
    bool IsValid() const;
    
    /// load mouse pointer textures
    void PreloadTextures(const Util::Array<Resources::ResourceId>& texResIds);
    /// update the mouse renderer
    void UpdatePointers(const Util::Array<CoreGraphics::MousePointer>& pointers);
    /// render mouse pointer(s)
    void RenderPointers();
    
protected:
    Util::Dictionary<Resources::ResourceId, Ptr<CoreGraphics::Texture> > textures;
    Util::Array<CoreGraphics::MousePointer> pointers;    
    bool isValid;    
};

//------------------------------------------------------------------------------
/**
*/
inline bool
MouseRenderDeviceBase::IsValid() const
{
    return this->isValid;
}

} // namespace Base
//------------------------------------------------------------------------------

    