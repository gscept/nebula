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
    Resources::ResourceName albedo;
    Resources::ResourceName normals;
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
    static void Setup(Resources::ResourceName heightMap, SizeT numGrassPlanesPerTuft, float grassPatchRadius);

    /// setup as grass
    static void SetupGrass(const Graphics::GraphicsEntityId id, const VegetationGrassSetup& setup);
    /// setup as mesh
    static void SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup);

    /// update resources
    static void UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
private:

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);

    enum class VegetationType : uint8
    {
        GrassType,      // use for grass and undergrowth
        MeshType        // use for rocks or other objects which might need meshes
    };

    enum
    {
        Vegetation_Mask,
        Vegetation_Albedos,
        Vegetation_Normals,
        Vegetation_Materials,
        Vegetation_Meshes,
        Vegetation_GPUInfo,
        Vegetation_TextureIndex,
        Vegetation_SlopeThreshold,
        Vegetation_HeightThreshold,
        Vegetation_Type
    };

    typedef Ids::IdAllocator<
        CoreGraphics::TextureId,
        CoreGraphics::TextureId,
        CoreGraphics::TextureId,
        CoreGraphics::TextureId,
        CoreGraphics::MeshId,
        uint,
        IndexT,
        float, float,
        VegetationType
    > VegetationAllocator;
    static VegetationAllocator vegetationAllocator;
};

} // namespace Vegetation
