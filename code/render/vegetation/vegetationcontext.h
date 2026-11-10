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

struct VegetationMeshSetup
{
    IO::Path mask = IO::Path::FolderAndFile("tex", "system/black");             // expects a 1 channel texture
    IO::Path model = IO::Path::FolderAndFile("mdl", "system/box");
    float slopeThreshold;                                                       // slope threshold in percentage
    float heightThreshold;                                                      // height threshold in percentage
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
    static uint SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup);

#if WITH_NEBULA_EDITOR
    /// Set the model
    static void SetModel(const Graphics::GraphicsEntityId id, uint index, const IO::Path& model);
    /// Set the mask to use for the vegetation
    static void SetMask(const Graphics::GraphicsEntityId id, uint index, CoreGraphics::TextureId mask);
#endif

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
        Vegetation_Material,
        Vegetation_Model,
        Vegetation_GPUInfo,
        Vegetation_SlopeThreshold,
        Vegetation_HeightThreshold
    };

    typedef Ids::IdAllocator<
        Materials::MaterialId,
        Models::ModelId,
        uint,
        float, 
        float
    > VegetationAllocator;
    static VegetationAllocator vegetationAllocator;
};

} // namespace Vegetation
