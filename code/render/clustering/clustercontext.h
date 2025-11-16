#pragma once
//------------------------------------------------------------------------------
/**
    Context handling GPU cluster culling

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/buffer.h"
#include "coregraphics/window.h"
#include <array>

namespace Clustering
{

class ClusterContext : public Graphics::GraphicsContext
{
    __DeclarePluginContext();
public:
    /// constructor
    ClusterContext();
    /// destructor
    virtual ~ClusterContext();

    /// setup light context using CameraSettings
    static void Create(float ZNear, float ZFar, const CoreGraphics::WindowId window);

    /// get number of clusters 
    static const SizeT GetNumClusters();
    /// get cluster dimensions
    static const std::array<SizeT, 3> GetClusterDimensions();

    /// update constants
    static void UpdateResources(const Graphics::FrameContext& ctx);
#ifndef PUBLIC_BUILD
    /// implement me
    static void OnRenderDebug(uint32_t flags);
#endif

    /// Update when window resized
    static void WindowResized(const CoreGraphics::WindowId id, SizeT width, SizeT height);

    /// Get cluster AABB buffer
    static const CoreGraphics::BufferId* GetClusterBuffer();
private:

    /// run light classification compute
    static void UpdateClusters();
};
} // namespace Clustering
