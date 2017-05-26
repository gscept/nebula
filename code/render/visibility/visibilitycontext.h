#pragma once
//------------------------------------------------------------------------------
/**
    @class Visibility::VisibilityContext
       
    (C) 2010 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "graphics/graphicsentity.h"
#include "math/bbox.h"
              
//------------------------------------------------------------------------------
namespace Visibility
{   
class VisibilityChecker;
class VisibilitySystemBase;

class VisibilityContext : public Core::RefCounted
{
    __DeclareClass(VisibilityContext);
public:
    /// constructor
    VisibilityContext();
    /// destructor
    virtual ~VisibilityContext();     
    /// get GfxEntity	
    const Ptr<Graphics::GraphicsEntity>& GetGfxEntity() const; 
    /// get BoundingBox	
    const Math::bbox& GetBoundingBox() const;  
    /// get frameid visible	
    IndexT GetVisibleFrameId() const;            
    /// set Visible
    void SetVisibleFrameId(IndexT frameId);
              
private:  
    friend class VisibilityChecker;
    /// setup
    void Setup(const Ptr<Graphics::GraphicsEntity>& entity); 
    /// update bounding box
    void UpdateBoundingBox(const Math::bbox& box);

    Ptr<Graphics::GraphicsEntity> gfxEntity;
    IndexT visibleFrameId;
    Math::bbox boundingBox;    
};

//------------------------------------------------------------------------------
/**
*/
inline IndexT 
VisibilityContext::GetVisibleFrameId() const
{
    return this->visibleFrameId;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
VisibilityContext::SetVisibleFrameId(IndexT val)
{
    this->visibleFrameId = val;
}  

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Graphics::GraphicsEntity>& 
VisibilityContext::GetGfxEntity() const
{
    return this->gfxEntity;        
}        

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox& 
VisibilityContext::GetBoundingBox() const
{
    return this->boundingBox;        
}
} // namespace Visibility
//------------------------------------------------------------------------------

