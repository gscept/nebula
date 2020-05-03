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
namespace Terrain
{

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
    static void Destroy();

    /// setup new terrain surface from texture and settings
    static void SetupTerrain(const Graphics::GraphicsEntityId entity, const Resources::ResourceName& texture, float minHeight, float maxHeight);

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

        CoreGraphics::VertexBufferId vbo;
        CoreGraphics::IndexBufferId ibo;
        CoreGraphics::VertexLayout vlo;
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