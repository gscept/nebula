#pragma once
//------------------------------------------------------------------------------
/**
    @class Graphics::View
    
    A view describes a camera which can observe a Stage. The view processes 
    the attached Stage through its FrameScript each frame.
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "frame/framescript.h"
#include "timing/time.h"
#include "graphicsentity.h"
#include "gpulang/render/system_shaders/shared.h"
namespace Graphics
{
class Camera;
class View : public Core::RefCounted
{
    __DeclareClass(View);
public:


    /// constructor
    View();
    /// destructor
    virtual ~View();

    /// Update constants
    void UpdateConstants();
    /// render through view, returns true if framescript needs resizing
    bool Render(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex);

    /// Set run function
    void SetFrameScript(bool(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex));
    /// Set viewport
    void SetViewport(const Math::rectangle<int>& rect);
    /// Get viewport
    const Math::rectangle<int>& GetViewport();

    /// set camera
    void SetCamera(const GraphicsEntityId& camera);
    /// get camera
    const GraphicsEntityId& GetCamera();

    /// Get reference to view constants
    Shared::ViewConstants::STRUCT& GetViewConstants();
    /// Get reference to shadow constants
    Shared::ShadowViewConstants::STRUCT& GetShadowConstants();

    /// set stage
    void SetStageMask(const uint16_t stage);
    /// get stage
    const uint16_t GetStageMask() const;

    /// returns whether view is enabled
    bool IsEnabled() const;
    
    /// enable this view
    void Enable();

    /// disable this view
    void Disable();
private:    
    friend class GraphicsServer;

    Shared::ViewConstants::STRUCT viewConstants;
    Shared::ShadowViewConstants::STRUCT shadowViewConstants;
    CoreGraphics::TextureId outputTarget;

    Math::rectangle<int> viewport;
    uint16_t stageMask;
    bool (*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex);
    GraphicsEntityId camera;
    bool enabled;
};

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetCamera(const GraphicsEntityId& camera)
{
    this->camera = camera;
}

//------------------------------------------------------------------------------
/**
*/
inline const GraphicsEntityId&
Graphics::View::GetCamera()
{
    return this->camera;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetStageMask(const uint16_t stageMask)
{
    this->stageMask = stageMask;
}

//------------------------------------------------------------------------------
/**
*/
inline const uint16_t
View::GetStageMask() const
{
    return this->stageMask;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
View::IsEnabled() const
{
    return this->enabled;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::Enable()
{
    this->enabled = true;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::Disable()
{
    this->enabled = false;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetFrameScript(bool(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex))
{
    this->func = func;
}

//------------------------------------------------------------------------------
/**
*/
inline void
View::SetViewport(const Math::rectangle<int>& rect)
{
    this->viewport = rect;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::rectangle<int>& 
View::GetViewport()
{
    return this->viewport;
}


//------------------------------------------------------------------------------
/**
*/
inline Shared::ViewConstants::STRUCT&
View::GetViewConstants()
{
    return this->viewConstants;
}

//------------------------------------------------------------------------------
/**
*/
inline Shared::ShadowViewConstants::STRUCT&
View::GetShadowConstants()
{
    return this->shadowViewConstants;
}

} // namespace Graphics
