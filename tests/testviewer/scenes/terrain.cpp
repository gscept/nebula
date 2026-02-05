#include "scenes.h"

#include "graphics/graphicsserver.h"
#include "models/modelcontext.h"
#include "lighting/lightcontext.h"
#include "visibility/visibilitycontext.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "terrain/terraincontext.h"

using namespace Timing;
using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace TerrainSceneData
{

Graphics::GraphicsEntityId entity;
Graphics::GraphicsEntityId tower;
Graphics::GraphicsEntityId ground;
Graphics::GraphicsEntityId terrain;
Graphics::GraphicsEntityId particle;
Util::Array<Graphics::GraphicsEntityId> entities;
Util::Array<Util::String> entityNames;
Util::Array<Graphics::GraphicsEntityId> pointLights;
Util::Array<Graphics::GraphicsEntityId> spotLights;
Util::Array<Graphics::GraphicsEntityId> decals;
Util::Array<Graphics::GraphicsEntityId> fogVolumes;
Util::Array<CoreGraphics::TextureId> decalTextures;

//------------------------------------------------------------------------------
/**
*/
static const char*
GraphicsEntityToName(GraphicsEntityId id)
{
    if (ModelContext::IsEntityRegistered(id)) return "Model";
    if (Lighting::LightContext::IsEntityRegistered(id)) return "Light";
    return "Entity";
}


//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    /*
    ground = Graphics::CreateEntity();
    Graphics::RegisterEntity<ModelContext, ObservableContext>(ground);
    ModelContext::Setup(ground, "mdl:environment/plcholder_world.n3", "Viewer");
    ModelContext::SetTransform(ground, Math::scaling(4) * Math::translation(0,0,0));
    entities.Append(ground);
    entityNames.Append("Ground");
    */

    terrain = Graphics::CreateEntity();
    Terrain::TerrainContext::RegisterEntity(terrain);

    Terrain::TerrainCreateInfo terrainInfo;
    terrainInfo.heightMap = "tex:terrain/everest Height Map (Merged)_PNG_BC4_1.dds";
    terrainInfo.decisionMap = "systex:black.dds";
    terrainInfo.enableRayTracing = false;
    terrainInfo.minHeight = 0.0f;
    terrainInfo.maxHeight = 1024.0f;
    terrainInfo.width = 8192.0f;
    terrainInfo.height = 8192.0f;
    terrainInfo.tileWidth = 256.0f;
    terrainInfo.tileHeight = 256.0f;
    terrainInfo.quadsPerTileX = 16.0f;
    terrainInfo.quadsPerTileY = 16.0f;
    Terrain::TerrainContext::SetupTerrain(terrain, terrainInfo);

    Terrain::BiomeParameters biomeParams =
    {
        .slopeThreshold = 0.5f,
        .heightThreshold = 900.0f,
        .uvScaleFactor = 64.0f
    };
    Terrain::BiomeSettings biomeParameters = Terrain::BiomeSettingsBuilder()
        .Parameters(biomeParams)
        .FlatMaterial(Terrain::BiomeMaterialBuilder().Finish())
        .SlopeMaterial(Terrain::BiomeMaterialBuilder().Finish())
        .HeightMaterial(Terrain::BiomeMaterialBuilder().Finish())
        .HeightSlopeMaterial(Terrain::BiomeMaterialBuilder().Finish())
        .Mask("tex:system/white.dds")
        .Finish();
    Terrain::TerrainContext::CreateBiome(terrain, biomeParameters);
};

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    n_error("implement me!");
}

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
}

//------------------------------------------------------------------------------
/**
*/
void RenderUI()
{
}

} // namespace ClusteredSceneData


Scene TerrainScene =
{
    "TerrainScene",
    TerrainSceneData::OpenScene,
    TerrainSceneData::CloseScene,
    TerrainSceneData::StepFrame,
    TerrainSceneData::RenderUI
};
