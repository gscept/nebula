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
#include "stage.h"
namespace Graphics
{
class Stage;
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
    /// render through view
    void Render(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex);

    /// Set run function
    void SetFrameScript(void(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex));
    /// Set viewport
    void SetViewport(const Math::rectangle<int>& rect);

    /// set camera
    void SetCamera(const GraphicsEntityId& camera);
    /// get camera
    const GraphicsEntityId& GetCamera();

    /// set stage
    void SetStage(const Ptr<Stage>& stage);
    /// get stage
    const Ptr<Stage>& GetStage() const;

    /// returns whether view is enabled
    bool IsEnabled() const;
    
    /// enable this view
    void Enable();

    /// disable this view
    void Disable();
private:    
    friend class GraphicsServer;

    Math::rectangle<int> viewport;
    void (*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex);
    GraphicsEntityId camera;
    Ptr<Stage> stage;
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
View::SetStage(const Ptr<Stage>& stage)
{
    this->stage = stage;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Stage>&
View::GetStage() const
{
    return this->stage;
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
View::SetFrameScript(void(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex))
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

} // namespace Graphics
