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
static constexpr StageMask ALL_STAGE_MASK = 0xFFFF;

static constexpr StageMask StageMaskFromIndex(const IndexT stageIndex)
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

} // namespace Graphics
