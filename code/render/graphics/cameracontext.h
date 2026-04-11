#pragma once
//------------------------------------------------------------------------------
/**
    @class Graphics::CameraContext

    Handles cameras

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "camerasettings.h"
#include "math/scalar.h"
namespace Graphics
{

class CameraContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// constructor
    CameraContext();
    /// destructor
    virtual ~CameraContext();

    /// create context
    static void Create();

    /// runs before frame is updated
    static void UpdateCameras(const Graphics::FrameContext& ctx);

    /// setup as projection and fov
    static void SetupProjectionFov(const Graphics::GraphicsEntityId id, float aspect, float fov, float znear, float zfar, const Graphics::StageMask = Graphics::PRIMARY_STAGE_MASK);
    /// setup as ortographic
    static void SetupOrthographic(const Graphics::GraphicsEntityId id, float width, float height, float znear, float zfar, const Graphics::StageMask = Graphics::PRIMARY_STAGE_MASK);

    /// set view
    static void SetView(const Graphics::GraphicsEntityId id, const Math::mat4& mat);
    /// get view
    static const Math::mat4& GetView(const Graphics::GraphicsEntityId id);
    /// get transform
    static const Math::mat4 GetTransform(const Graphics::GraphicsEntityId id);

    /// get projection
    static const Math::mat4& GetProjection(const Graphics::GraphicsEntityId id);
    /// get view-projection
    static const Math::mat4& GetViewProjection(const Graphics::GraphicsEntityId id);
    /// get settings
    static const CameraSettings& GetSettings(const Graphics::GraphicsEntityId id);
    /// Get stage mask
    static Graphics::StageMask GetStageMask(const Graphics::GraphicsEntityId id);

    /// Get all cameras affecting LOD
    static const Util::Array<Graphics::GraphicsEntityId>& GetLODCameras();
    /// Add a camera to affect lod
    static void AddLODCamera(const Graphics::GraphicsEntityId id);
    /// Remove a camera from affecting lod
    static void RemoveLODCamera(const Graphics::GraphicsEntityId id);

    /// get the LOD camera
    static Graphics::GraphicsEntityId GetLODCamera();
    /// set the LOD camera
    static void SetLODCamera(const Graphics::GraphicsEntityId id);

private:

    enum
    {
        Camera_Settings,
        Camera_Projection,
        Camera_View,
        Camera_ViewProjection,
        Camera_StageMask
    };
    typedef Ids::IdAllocator<
        Graphics::CameraSettings,
        Math::mat4,             // projection
        Math::mat4,             // view-transform
        Math::mat4,             // view-projection
        Graphics::StageMask     // Mask indicating the stages this camera exists in
    > CameraAllocator;
    static CameraAllocator cameraAllocator;

    static Util::Array<Graphics::GraphicsEntityId> LodCameras;
    static Graphics::GraphicsEntityId lodCamera;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(const Graphics::ContextEntityId id);
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
CameraContext::Alloc()
{
    return cameraAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
inline void
CameraContext::Dealloc(const Graphics::ContextEntityId id)
{
    cameraAllocator.Dealloc(id.id);
}

} // namespace Graphics
