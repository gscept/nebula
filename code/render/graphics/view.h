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
#include "ids/id.h"
#include "coregraphics/config.h"
#include "frame/framescript.h"
#include "timing/time.h"
#include "graphicsentity.h"
#include "gpulang/render/system_shaders/shared.h"
namespace Graphics
{

typedef uint16_t StageMask;
static constexpr StageMask PRIMARY_STAGE_MASK = 0x1;
static constexpr StageMask SHADOW_STAGE_MASK = 0x2;
static constexpr StageMask DEFAULT_STAGE_MASK = PRIMARY_STAGE_MASK | SHADOW_STAGE_MASK;

static constexpr StageMask GetStageMask(const StageMask stageIndex)
{
    return 1 << (stageIndex << SHADOW_STAGE_MASK);
}

ID_32_TYPE(ViewId);

struct ViewCreateInfo
{
    bool(*frameScript)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex) = nullptr;
    Math::rectangle<int> viewport;
    Graphics::GraphicsEntityId camera = Graphics::InvalidGraphicsEntityId;
    Graphics::StageMask stageMask = Graphics::PRIMARY_STAGE_MASK;
};


/// Create a new view
ViewId CreateView(const ViewCreateInfo& info);
/// Destroy view
void DestroyView(const ViewId id);

/// Apply view
void ViewApply(const ViewId id);
/// Render view
bool ViewRender(const ViewId id, const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex);
/// Set view frame script
void ViewSetFrameScript(const ViewId id, bool(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex));
/// Set view viewport
void ViewSetViewport(const ViewId id, const Math::rectangle<int>& rect);
/// Get view viewport
const Math::rectangle<int>& ViewGetViewport(const ViewId id);
/// Set view camera
void ViewSetCamera(const ViewId id, const GraphicsEntityId& camera);
/// Get view camera
const GraphicsEntityId& ViewGetCamera(const ViewId id);
/// Get view constants
Shared::ViewConstants::STRUCT& ViewGetViewConstants(const ViewId id);
/// Get shadow view constants
Shared::ShadowViewConstants::STRUCT& ViewGetShadowConstants(const ViewId id);
/// Set view stage
void ViewSetStageMask(const ViewId id, const Graphics::StageMask stage);
/// Get view stage
const Graphics::StageMask ViewGetStageMask(const ViewId id);
/// Enable view
void ViewEnable(const ViewId id);
/// Disable view
void ViewDisable(const ViewId id);
/// Check if view is enabled
bool ViewIsEnabled(const ViewId id);

enum
{
    View_ViewConstants,
    View_ShadowConstants,
    View_OutputTarget,
    View_Viewport,
    View_StageMask,
    View_FrameScriptFunc,
    View_Camera,
    View_Enabled
};

typedef Ids::IdAllocator<
    Shared::ViewConstants::STRUCT,
    Shared::ShadowViewConstants::STRUCT,
    CoreGraphics::TextureId,

    Math::rectangle<int>,
    Graphics::StageMask,
    bool (*)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex),
    GraphicsEntityId,
    bool
> ViewAllocator;
extern ViewAllocator viewAllocator;


//class View : public Core::RefCounted
//{
//    __DeclareClass(View);
//public:
//
//
//    /// constructor
//    View();
//    /// destructor
//    virtual ~View();
//
//    /// Update constants
//    void UpdateConstants();
//    /// render through view, returns true if framescript needs resizing
//    bool Render(const IndexT frameIndex, const Timing::Time time, const IndexT bufferIndex);
//
//    /// Set run function
//    void SetFrameScript(bool(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex));
//    /// Set viewport
//    void SetViewport(const Math::rectangle<int>& rect);
//    /// Get viewport
//    const Math::rectangle<int>& GetViewport();
//
//    /// set camera
//    void SetCamera(const GraphicsEntityId& camera);
//    /// get camera
//    const GraphicsEntityId& GetCamera();
//
//    /// Get reference to view constants
//    Shared::ViewConstants::STRUCT& GetViewConstants();
//    /// Get reference to shadow constants
//    Shared::ShadowViewConstants::STRUCT& GetShadowConstants();
//
//    /// set stage
//    void SetStageMask(const Graphics::StageMask stage);
//    /// get stage
//    const Graphics::StageMask GetStageMask() const;
//
//    /// returns whether view is enabled
//    bool IsEnabled() const;
//    
//    /// enable this view
//    void Enable();
//
//    /// disable this view
//    void Disable();
//private:    
//    friend class GraphicsServer;
//
//    Shared::ViewConstants::STRUCT viewConstants;
//    Shared::ShadowViewConstants::STRUCT shadowViewConstants;
//    CoreGraphics::TextureId outputTarget;
//
//    Math::rectangle<int> viewport;
//    Graphics::StageMask stageMask;
//    bool (*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex);
//    GraphicsEntityId camera;
//    bool enabled;
//};
//
////------------------------------------------------------------------------------
///**
//*/
//inline void
//View::SetCamera(const GraphicsEntityId& camera)
//{
//    this->camera = camera;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline const GraphicsEntityId&
//Graphics::View::GetCamera()
//{
//    return this->camera;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline void
//View::SetStageMask(const Graphics::StageMask stageMask)
//{
//    this->stageMask = stageMask;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline const Graphics::StageMask
//View::GetStageMask() const
//{
//    return this->stageMask;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline bool
//View::IsEnabled() const
//{
//    return this->enabled;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline void
//View::Enable()
//{
//    this->enabled = true;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline void
//View::Disable()
//{
//    this->enabled = false;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline void
//View::SetFrameScript(bool(*func)(const Math::rectangle<int>& viewport, IndexT frameIndex, IndexT bufferIndex))
//{
//    this->func = func;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline void
//View::SetViewport(const Math::rectangle<int>& rect)
//{
//    this->viewport = rect;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline const Math::rectangle<int>& 
//View::GetViewport()
//{
//    return this->viewport;
//}
//
//
////------------------------------------------------------------------------------
///**
//*/
//inline Shared::ViewConstants::STRUCT&
//View::GetViewConstants()
//{
//    return this->viewConstants;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//inline Shared::ShadowViewConstants::STRUCT&
//View::GetShadowConstants()
//{
//    return this->shadowViewConstants;
//}


} // namespace Graphics
