#pragma once
//------------------------------------------------------------------------------
/**
    Volumetric fog implements a ray marching technique 
    through the scene from the point of view from the camera.

    There are two types of fog, atmospheric fog which is persistent
    and controlled through global parameters, and local fog volumes 
    which are individually configurable. 

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
        const Math::matrix44& transform,
        const float density,
        const Math::float4 absorption);
    /// setup a new volume as a sphere
    static void SetupSphereVolume(
        const Graphics::GraphicsEntityId id,
        Math::float4 position,
        float radius,
        const float density,
        const Math::float4 absorption);

    /// prepare light lists
    static void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

    /// set global turbidity (fog density)
    void SetGlobalTurbidity(float f);
    /// set the global fog absorption color
    void SetGlobalAbsorption(const Math::float4& color);

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
        FogVolume_Density,
        FogVolume_Absorption,
    };
    typedef Ids::IdAllocator<
        FogVolumeType,
        Ids::Id32,
        float,
        Math::float4 
    > FogGenericVolumeAllocator;
    static FogGenericVolumeAllocator fogGenericVolumeAllocator;

    enum
    {
        FogBoxVolume_Transform,
    };
    typedef Ids::IdAllocator<
        Math::matrix44
    > FogBoxVolumeAllocator;
    static FogBoxVolumeAllocator fogBoxVolumeAllocator;

    enum
    {
        FogSphereVolume_Position,
        FogSphereVolume_Radius,
    };
    typedef Ids::IdAllocator<
        Math::float4,
        float
    > FogSphereVolumeAllocator;
    static FogSphereVolumeAllocator fogSphereVolumeAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Fog
