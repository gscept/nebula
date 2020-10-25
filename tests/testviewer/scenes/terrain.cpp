#include "stdneb.h"
#include "scenes.h"

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

    Terrain::TerrainSetupSettings settings{
            0, 1024.0f,      // min/max height
            8192, 8192,   // world size in meters
            256, 256,     // tile size in meters
            16, 16        // 1 vertex every X meters
    };
    Terrain::TerrainContext::SetupTerrain(terrain,
        "tex:terrain/everest Height Map (Merged)_PNG_BC4_1.dds",
        "tex:system/black.dds",
        "tex:terrain/dirt_aerial_02_diff_2k.dds",
        settings);

    Terrain::BiomeSetupSettings biomeSettings =
    {
        0.5f, 900.0f, 64.0f
    };
    Terrain::TerrainContext::CreateBiome(biomeSettings,
        {
            "tex:terrain/base_material/brown_mud_leaves_01_diff_2k_PNG_BC7_1.dds",
            "tex:terrain/base_material/brown_mud_leaves_01_nor_2k_PNG_BC5_1.dds",
            "tex:terrain/base_material/brown_mud_leaves_01_material_2k_PNG_BC7_1.dds"
        },
        {
            "tex:terrain/base_material/dirt_aerial_02_diff_2k_PNG_BC7_1.dds",
            "tex:terrain/base_material/dirt_aerial_02_nor_2k_PNG_BC5_1.dds",
            "tex:terrain/base_material/dirt_aerial_02_material_2k_PNG_BC7_1.dds"
        },
        {
            "tex:terrain/base_material/snow_02_albedo_2k_PNG_BC7_1.dds",
            "tex:terrain/base_material/snow_02_nor_2k_PNG_BC5_1.dds",
            "tex:terrain/base_material/snow_02_material_2k_PNG_BC7_1.dds"
        },
        {
            "tex:terrain/base_material/rock_ground_02_albedo_2k_PNG_BC7_1.dds",
            "tex:terrain/base_material/rock_ground_02_nor_2k_PNG_BC5_1.dds",
            "tex:terrain/base_material/rock_ground_02_material_2k_PNG_BC7_1.dds"
        },
        "tex:system/white.dds"
    );
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
