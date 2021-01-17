#pragma once
//------------------------------------------------------------------------------
/**
    Volumetric fog implements a ray marching technique 
    through the scene from the point of view from the camera.

    There are two types of fog, atmospheric fog which is persistent
    and controlled through global parameters, and local fog volumes 
    which are individually configurable. 

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
namespace Fog
{

class VolumetricFogContext : public Graphics::GraphicsContext
{
    _DeclareContext()
public:

    /// constructor
    VolumetricFogContext();
    /// destructor
    virtual ~VolumetricFogContext();

    /// setup volumetric fog context
    static void Create();
    /// discard volumetric fog context
    static void Discard();

    /// setup a new volume as a box
    static void SetupBoxVolume(
        const Graphics::GraphicsEntityId id,
        const Math::mat4& transform,
        const float density,
        const Math::vec3& absorption);
    /// set box transform
    static void SetBoxTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform);


    /// setup a new volume as a sphere
    static void SetupSphereVolume(
        const Graphics::GraphicsEntityId id,
        const Math::vec3& position,
        float radius,
        const float density,
        const Math::vec3& absorption);
    /// set sphere position
    static void SetSpherePosition(const Graphics::GraphicsEntityId id, const Math::vec3& position);
    /// set sphere radius
    static void SetSphereRadius(const Graphics::GraphicsEntityId id, const float radius);

    /// set volume turbidity
    static void SetTurbidity(const Graphics::GraphicsEntityId id, const float turbidity);
    /// set volume absorption
    static void SetAbsorption(const Graphics::GraphicsEntityId id, const Math::vec3& absorption);

    /// prepare light lists
    static void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// render IMGUI
    static void RenderUI(const Graphics::FrameContext& ctx);

    /// set global turbidity (fog density)
    void SetGlobalTurbidity(float f);
    /// set the global fog absorption color
    void SetGlobalAbsorption(const Math::vec3& color);

    /// get transform
    static Math::mat4 GetTransform(const Graphics::GraphicsEntityId id);
    /// set transform
    static void SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& mat);

private:

    /// cull local volumes
    static void CullAndClassify();
    /// run ray marching algorithm
    static void Render();

    enum FogVolumeType
    {
        BoxVolume,
        SphereVolume
    };
    enum
    {
        FogVolume_Type,
        FogVolume_TypedId,
        FogVolume_Turbidity,
        FogVolume_Absorption,
    };
    typedef Ids::IdAllocator<
        FogVolumeType,
        Ids::Id32,
        float,
        Math::vec3 
    > FogGenericVolumeAllocator;
    static FogGenericVolumeAllocator fogGenericVolumeAllocator;

    enum
    {
        FogBoxVolume_Transform,
    };
    typedef Ids::IdAllocator<
        Math::mat4
    > FogBoxVolumeAllocator;
    static FogBoxVolumeAllocator fogBoxVolumeAllocator;

    enum
    {
        FogSphereVolume_Position,
        FogSphereVolume_Radius,
    };
    typedef Ids::IdAllocator<
        Math::vec3,
        float
    > FogSphereVolumeAllocator;
    static FogSphereVolumeAllocator fogSphereVolumeAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Fog
