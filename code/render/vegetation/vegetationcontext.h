#pragma once
//------------------------------------------------------------------------------
/**
    The vegetation context handles rendering of grass, trees and other types of vegetation

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "resources/resource.h"
#include "ids/idallocator.h"

namespace Vegetation
{

ID_32_TYPE(VegetationBiomeId);
struct VegetationSetupSettings
{
    float minHeight, maxHeight;

    Math::vec2 worldSize;
};

struct VegetationGrassSetup
{
    Resources::ResourceName mask;           // expects a 1 channel texture
    Resources::ResourceName albedo;
    Resources::ResourceName normals;
    Resources::ResourceName material;
    float slopeThreshold;                   // slope threshold in percentage
    float heightThreshold;                  // height threshold in percentage
};

struct VegetationMeshSetup
{
    Resources::ResourceName mask;           // expects a 1 channel texture
    Resources::ResourceName material;
    Resources::ResourceName mesh;
    float slopeThreshold;                   // slope threshold in percentage
    float heightThreshold;                  // height threshold in percentage
};

class VegetationContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// constructor
    VegetationContext();
    /// destructor
    ~VegetationContext();

    /// create vegetation context
    static void Create(const VegetationSetupSettings& settings);
    /// discard vegetation context
    static void Discard();

    /// Setup vegetation context
    static void Setup(Resources::ResourceName heightMap);

    /// setup as mesh
    static void SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup);

    /// update resources
    static void UpdateViewResources(const Graphics::ViewId view, const Graphics::FrameContext& ctx);
private:

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);

    enum LoadedBits
    {
        Model,
        Material
    };

    enum
    {
        Vegetation_Mask,
        Vegetation_Material,
        Vegetation_Mesh,
        Vegetation_GPUInfo,
        Vegetation_SlopeThreshold,
        Vegetation_HeightThreshold,
        Vegetation_LoadedBits
    };

    typedef Ids::IdAllocator<
        CoreGraphics::TextureId,
        Materials::MaterialId,
        CoreGraphics::MeshId,
        uint,
        float, 
        float,
        uint
    > VegetationAllocator;
    static VegetationAllocator vegetationAllocator;
};

} // namespace Vegetation
