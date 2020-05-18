#pragma once
//------------------------------------------------------------------------------
/**
    Terrain rendering subsystem

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "resources/resourceid.h"
#include "math/bbox.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/texture.h"
#include "coregraphics/resourcetable.h"
namespace Terrain
{

struct TerrainSetupSettings
{
    float minHeight, maxHeight;
    float worldSizeX, worldSizeZ;
    SizeT tileWidth, tileHeight;
    float vertexDensityX, vertexDensityY; // vertex density is vertices per meter
};

class TerrainContext : public Graphics::GraphicsContext
{
    _DeclareContext();
public:

    /// constructor
    TerrainContext();
    /// destructor
    virtual ~TerrainContext();

    /// create terrain context
    static void Create();
    /// destroy terrain context
    static void Discard();

    /// setup new terrain surface from texture and settings
    static void SetupTerrain(
        const Graphics::GraphicsEntityId entity, 
        const Resources::ResourceName& heightMap, 
        const Resources::ResourceName& normalMap, 
        const Resources::ResourceName& decisionMap,
        const TerrainSetupSettings& settings);

    /// cull terrain patches
    static void CullPatches(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// update virtual texture
    static void UpdateVirtualTexture(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

private:

    struct TerrainLoadInfo
    {
        Resources::ResourceName texture;
        float minHeight, maxHeight;
    };

    struct TerrainRuntimeInfo
    {
        Util::Array<Math::bbox> sectionBoxes;
        Util::Array<CoreGraphics::PrimitiveGroup> sectorPrimGroups;
        Util::Array<bool> sectorVisible;
        Util::Array<Util::FixedArray<bool>> sectorLodResidency;
        Util::Array<uint> sectorUniformOffsets;
        Util::Array<Math::float2> sectorUv;
        Util::Array<float> sectorLod;

        float heightMapWidth, heightMapHeight;
        float worldWidth, worldHeight;
        float maxHeight, minHeight;
        CoreGraphics::TextureId heightMap;
        CoreGraphics::TextureId normalMap;
        CoreGraphics::TextureId decisionMap;

        CoreGraphics::VertexBufferId vbo;
        CoreGraphics::IndexBufferId ibo;

        CoreGraphics::ResourceTableId patchTable;
    };

    enum
    {
        Terrain_LoadInfo,
        Terrain_RuntimeInfo
    };

    typedef Ids::IdAllocator<
        TerrainLoadInfo,
        TerrainRuntimeInfo
    > TerrainAllocator;
    static TerrainAllocator terrainAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Terrain
