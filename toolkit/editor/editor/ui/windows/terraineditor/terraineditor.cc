//------------------------------------------------------------------------------
//  terraineditor.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "terraineditor.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "editor/ui/windowserver.h"
#include "io/ioserver.h"
#include "io/fswrapper.h"
#include "imgui_internal.h"
#include "editor/tools/selectioncontext.h"
#include "terrain/terraincontext.h"
#include "graphicsfeature/components/terrain.h"
#include "editor/tools/pathconverter.h"
#include "resources/resourceserver.h"
#include "coregraphics/meshresource.h"
#include "graphicsfeature/graphicsfeatureunit.h"

#include "coregraphics/pipeline.h"

#include "dynui/imguicontext.h"

#include "frame/editorframe.h"
#include "frame/default.h"

#include "input/inputserver.h"
#include "input/mouse.h"

#include "tinyfiledialogs.h"

#include "gpulang/editor/editor/ui/windows/terraineditor/terrainbrush.h"

using namespace Editor;

struct BiomeTextures
{
    Dynui::ImguiTextureId mask;
    CoreGraphics::TextureId maskTex;
    Util::FixedArray<Resources::ResourceName> albedoPaths;
    Util::FixedArray<Resources::ResourceId> albedoResources;
    Util::FixedArray<Dynui::ImguiTextureId> albedo;
    Util::FixedArray<Resources::ResourceName> normalsPaths;
    Util::FixedArray<Resources::ResourceId> normalsResources;
    Util::FixedArray<Dynui::ImguiTextureId> normals;
    Util::FixedArray<Resources::ResourceName> materialPaths;
    Util::FixedArray<Resources::ResourceId> materialResources;
    Util::FixedArray<Dynui::ImguiTextureId> material;

    BiomeTextures()
    {
        this->albedoPaths.Resize(4);
        this->albedoResources.Resize(4);
        this->albedo.Resize(4);
        this->normals.Resize(4);
        this->normalsResources.Resize(4);
        this->normalsPaths.Resize(4);
        this->material.Resize(4);
        this->materialResources.Resize(4);
        this->materialPaths.Resize(4);
    }
};

struct
{
    float brushSize = 16.0f;
    float brushHardness = 0.5f;
    float brushFalloff = 0.5f;

    bool invalidateBrush = false;
    bool paint = false;
    bool drawBrushPreview = true;
    Terrainbrush::BrushGenerationShape brushShape = Terrainbrush::Circle;

    Terrainbrush::BrushUniforms::STRUCT brushUniforms;
    CoreGraphics::ResourceTableId brushResourceTable;
    CoreGraphics::BufferId brushUniformBuffer;

    CoreGraphics::ShaderId brushShader;
    CoreGraphics::ShaderProgramId brushHeightShaderProgram, brushMaskShaderProgram;

    CoreGraphics::ShaderProgramId brushPreviewProgram;
    CoreGraphics::PipelineId brushPreviewPipeline;

    Resources::ResourceId brushMeshResource;
    CoreGraphics::MeshResourceId brushMesh;

    CoreGraphics::ShaderProgramId brushGenerationProgram;
    CoreGraphics::TextureId brushTexture;
    Terrainbrush::BrushGenerationUniforms::STRUCT brushGenerationUniforms;
    CoreGraphics::BufferId brushGenerationUniformBuffer;

    Util::Array<Util::String> biomeNames;
    Util::Array<Terrain::TerrainBiomeId> biomes;
    Util::Array<BiomeTextures> biomeTextures;
    Util::Array<Terrain::BiomeSettings> biomeSettings;

    CoreGraphics::TextureId activeHeightMap, activeBiomeMask;
} terrainEditorState;

namespace Presentation
{
__ImplementClass(Presentation::TerrainEditor, 'TrEd', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
TerrainEditor::TerrainEditor()
{
    terrainEditorState.brushShader = CoreGraphics::ShaderGet("shd:editor/ui/windows/terraineditor/terrainbrush.gplb");
    terrainEditorState.brushHeightShaderProgram = CoreGraphics::ShaderGetProgram(terrainEditorState.brushShader, CoreGraphics::ShaderFeatureMask("BrushHeight"));
    terrainEditorState.brushMaskShaderProgram = CoreGraphics::ShaderGetProgram(terrainEditorState.brushShader, CoreGraphics::ShaderFeatureMask("BrushMask"));

    terrainEditorState.brushGenerationProgram = CoreGraphics::ShaderGetProgram(terrainEditorState.brushShader, CoreGraphics::ShaderFeatureMask("BrushGenerate"));
    terrainEditorState.brushPreviewProgram = CoreGraphics::ShaderGetProgram(terrainEditorState.brushShader, CoreGraphics::ShaderFeatureMask("Preview"));
    terrainEditorState.brushResourceTable = CoreGraphics::ShaderCreateResourceTable(terrainEditorState.brushShader, NEBULA_BATCH_GROUP);

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.elementSize = sizeof(Terrainbrush::BrushUniforms::STRUCT);
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    bufInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
    terrainEditorState.brushUniformBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.elementSize = sizeof(Terrainbrush::BrushGenerationUniforms::STRUCT);
    terrainEditorState.brushGenerationUniformBuffer = CoreGraphics::CreateBuffer(bufInfo);

    CoreGraphics::TextureCreateInfo brushTexInfo;
    brushTexInfo.width = 512;
    brushTexInfo.height = 512;
    brushTexInfo.format = CoreGraphics::PixelFormat::R32F;
    brushTexInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    terrainEditorState.brushTexture = CoreGraphics::CreateTexture(brushTexInfo);

    CoreGraphics::ResourceTableSetConstantBuffer(terrainEditorState.brushResourceTable, {
        terrainEditorState.brushUniformBuffer, Terrainbrush::BrushUniforms::BINDING
    });

    CoreGraphics::ResourceTableSetConstantBuffer(terrainEditorState.brushResourceTable, {
        terrainEditorState.brushGenerationUniformBuffer, Terrainbrush::BrushGenerationUniforms::BINDING
    });

    CoreGraphics::ResourceTableSetRWTexture(terrainEditorState.brushResourceTable, {
        terrainEditorState.brushTexture, Terrainbrush::BrushOutput::BINDING
    });

    CoreGraphics::ResourceTableSetTexture(terrainEditorState.brushResourceTable, {
        terrainEditorState.brushTexture, Terrainbrush::Brush::BINDING
    });

    CoreGraphics::ResourceTableCommitChanges(terrainEditorState.brushResourceTable);

    // this->additionalFlags = ImGuiWindowFlags_MenuBar;
    FrameScript_editorframe::RegisterSubgraph_TerrainTool_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (terrainEditorState.invalidateBrush)
        {
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainEditorState.brushTexture);
            CoreGraphics::CmdSetShaderProgram(cmdBuf, terrainEditorState.brushGenerationProgram);
            CoreGraphics::CmdSetResourceTable(cmdBuf, terrainEditorState.brushResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::CmdDispatch(cmdBuf, Math::divandroundup(dims.width, 8), Math::divandroundup(dims.height, 8), 1);
            terrainEditorState.invalidateBrush = false;
        }

        if (terrainEditorState.paint)
        {
            if (terrainEditorState.brushUniforms.mode == Terrainbrush::Biome)
                CoreGraphics::CmdSetShaderProgram(cmdBuf, terrainEditorState.brushMaskShaderProgram);
            else
                CoreGraphics::CmdSetShaderProgram(cmdBuf, terrainEditorState.brushHeightShaderProgram);

            CoreGraphics::CmdSetResourceTable(cmdBuf, terrainEditorState.brushResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
            CoreGraphics::CmdDispatch(cmdBuf, Math::divandroundup(terrainEditorState.brushSize, 8), Math::divandroundup(terrainEditorState.brushSize, 8), 1);
            terrainEditorState.paint = false;
        }
    }, nullptr, nullptr);

    FrameScript_default::RegisterSubgraphPipelines_TerrainEditorBrush_Pass([](const CoreGraphics::PassId pass, uint subpass)
    {
        if (terrainEditorState.brushPreviewPipeline != CoreGraphics::InvalidPipelineId)
            DestroyGraphicsPipeline(terrainEditorState.brushPreviewPipeline);
        terrainEditorState.brushPreviewPipeline = CoreGraphics::CreateGraphicsPipeline(
            {
                .shader = terrainEditorState.brushPreviewProgram,
                .pass = pass,
                .subpass = subpass,
                .inputAssembly = CoreGraphics::InputAssemblyKey{ {.topo = CoreGraphics::PrimitiveTopology::TriangleList, .primRestart = false } }
            });
    });

    // Bleh, run this again
    FrameScript_default::SetupPipelines();
    FrameScript_default::RegisterSubgraph_TerrainEditorBrush_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!terrainEditorState.drawBrushPreview)
            return;
        terrainEditorState.brushUniforms.viewport[0] = viewport.width();
        terrainEditorState.brushUniforms.viewport[1] = viewport.height();
        terrainEditorState.brushUniforms.invViewport[0] = 1.0f / viewport.width();
        terrainEditorState.brushUniforms.invViewport[1] = 1.0f / viewport.height();
        CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, terrainEditorState.brushPreviewPipeline);
        CoreGraphics::CmdSetResourceTable(cmdBuf, terrainEditorState.brushResourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
        CoreGraphics::MeshId mesh = CoreGraphics::MeshResourceGetMesh(terrainEditorState.brushMesh, 0);
        CoreGraphics::MeshBind(mesh, cmdBuf);

        CoreGraphics::CmdDraw(cmdBuf, CoreGraphics::MeshGetPrimitiveGroup(mesh, 0));
    });

    terrainEditorState.brushMeshResource = Resources::CreateResource("sysmsh:cube.nvx", "system", nullptr, nullptr, true, false);
    terrainEditorState.brushMesh = CoreGraphics::MeshResourceId(terrainEditorState.brushMeshResource);
}

//------------------------------------------------------------------------------
/**
*/
TerrainEditor::~TerrainEditor()
{
    // empty
}

// Create radio button effect with visual feedback
auto drawModeButton = [](const char* label, auto currentMode, auto buttonMode) -> bool
{
    bool isActive = currentMode == buttonMode;
    if (isActive)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }

    bool clicked = ImGui::Button(label);

    if (isActive)
    {
        ImGui::PopStyleColor();
    }

    return clicked;
};

//------------------------------------------------------------------------------
/**
*/
void 
TerrainEditor::Run(SaveMode save)
{
    auto const& selection = Tools::SelectionContext::Selection();
    if (selection.Size() != 1)
    {
        return;
    }

    Editor::Entity entity = selection[0];

    Game::Entity ent = Editor::state.editables[entity.index].gameEntity;
    Game::World* world = Game::GetWorld(ent.world);
    Game::EntityMapping entityMapping = world->GetEntityMapping(ent);
    auto const& components = world->GetDatabase()->GetTable(entityMapping.table).GetAttributes();

    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Math::vec2 mousePos = mouse->GetPixelPosition();
    terrainEditorState.brushUniforms.size[0] = terrainEditorState.brushSize;
    terrainEditorState.brushUniforms.size[1] = terrainEditorState.brushSize;
    CoreGraphics::BufferUpdate(terrainEditorState.brushUniformBuffer, terrainEditorState.brushUniforms);

    terrainEditorState.paint = mouse->ButtonPressed(Input::MouseButton::LeftButton);
    auto terrainComponent = Game::GetComponentId<GraphicsFeature::Terrain>();
    for (int i = 0; i < components.Size(); i++)
    {
        auto component = components[i];

        if (component != terrainComponent)
        {
            continue;
        }

        auto terrain = world->GetComponent<GraphicsFeature::Terrain>(ent);
        if (ImGui::CollapsingHeader("Terrain Generation"))
        {
            ImGui::SliderFloat("Min Height", &terrain.minHeight, 0.0f, 1024.0f);
            ImGui::SliderFloat("Max Height", &terrain.maxHeight, 0.0f, 1024.0f);
            ImGui::SliderFloat("Quads Per Tile Horizontal", &terrain.quadsPerTileX, 16, 512);
            ImGui::SliderFloat("Quads Per Tile Vertical", &terrain.quadsPerTileZ, 16, 512);
            ImGui::SliderFloat("Tile Height", &terrain.tileHeight, 16, 512);
            ImGui::SliderFloat("Tile Width", &terrain.tileWidth, 16, 512);
            ImGui::SliderFloat("World Width", &terrain.worldSizeX, 1024, 16384);
            ImGui::SliderFloat("World Height", &terrain.worldSizeZ, 1024, 16384);
            Editor::state.editorWorld->SetComponent<GraphicsFeature::Terrain>(entity, terrain);
            Editor::state.editorWorld->MarkAsModified(entity);

            ImGui::NewLine();
            if (ImGui::Button("Generate Terrain"))
            {
                Terrain::TerrainCreateInfo info;
                info.minHeight = terrain.minHeight;
                info.maxHeight = terrain.maxHeight;
                info.quadsPerTileX = terrain.quadsPerTileX;
                info.quadsPerTileY = terrain.quadsPerTileZ;
                info.tileWidth = terrain.tileWidth;
                info.tileHeight = terrain.tileHeight;
                info.width = terrain.worldSizeX;
                info.height = terrain.worldSizeZ;
                
                Terrain::TerrainContext::SetupTerrain(terrain.graphicsEntityId, info);
            }
        }

        terrainEditorState.brushUniforms.minHeight = terrain.minHeight;
        terrainEditorState.brushUniforms.maxHeight = terrain.maxHeight;
        terrainEditorState.invalidateBrush = true;
        CoreGraphics::TextureDimensions brushDims = CoreGraphics::TextureGetDimensions(terrainEditorState.brushTexture);

        terrainEditorState.brushUniforms.brushTextureSize[0] = brushDims.width;
        terrainEditorState.brushUniforms.brushTextureSize[1] = brushDims.height;

        if (terrainEditorState.invalidateBrush)
        {
            terrainEditorState.brushGenerationUniforms.size[0] = brushDims.width;
            terrainEditorState.brushGenerationUniforms.size[1] = brushDims.height;
            terrainEditorState.brushGenerationUniforms.hardness = terrainEditorState.brushHardness;
            terrainEditorState.brushGenerationUniforms.shape = terrainEditorState.brushShape;
            CoreGraphics::BufferUpdate(terrainEditorState.brushGenerationUniformBuffer, terrainEditorState.brushGenerationUniforms);
        }
        if (ImGui::CollapsingHeader("Heightmap", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (terrainEditorState.activeHeightMap != CoreGraphics::InvalidTextureId)
            {
                static Dynui::ImguiTextureId textureInfo;
                textureInfo.nebulaHandle = terrainEditorState.activeHeightMap;
                textureInfo.mip = 0;
                textureInfo.layer = 0;
                textureInfo.rangeMin = terrain.minHeight;
                textureInfo.rangeMax = terrain.maxHeight;
                textureInfo.splat = 1;

                ImVec2 imageSize = { 128.0f, 128.0f };

                ImGui::Image((void*)&textureInfo, imageSize);
            }

            if (ImGui::Button("New Heightmap"))
            {
                if (terrainEditorState.activeHeightMap != CoreGraphics::InvalidTextureId)
                {
                    CoreGraphics::DestroyTexture(terrainEditorState.activeHeightMap);
                    terrainEditorState.activeHeightMap = CoreGraphics::InvalidTextureId;
                }

                CoreGraphics::TextureCreateInfo texInfo;
                texInfo.name = "Editor Height Map";
                texInfo.width = terrain.worldSizeX;
                texInfo.height = terrain.worldSizeZ;
                texInfo.format = CoreGraphics::PixelFormat::R16F;
                texInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::ReadWrite | CoreGraphics::TextureUsage::TransferSource;
                terrainEditorState.activeHeightMap = CoreGraphics::CreateTexture(texInfo);
                Terrain::TerrainContext::SetHeightmap(terrain.graphicsEntityId, terrainEditorState.activeHeightMap);

                terrainEditorState.brushUniforms.heightmapSize[0] = terrain.worldSizeX;
                terrainEditorState.brushUniforms.heightmapSize[1] = terrain.worldSizeZ;

                CoreGraphics::ResourceTableSetRWTexture(terrainEditorState.brushResourceTable, {
                    terrainEditorState.activeHeightMap, Terrainbrush::OutputHeight::BINDING
                });

                CoreGraphics::ResourceTableCommitChanges(terrainEditorState.brushResourceTable);
            }

            if (terrainEditorState.activeHeightMap != CoreGraphics::InvalidTextureId)
            {
                if (drawModeButton("Raise", terrainEditorState.brushUniforms.mode, Terrainbrush::Raise))
                {
                    terrainEditorState.brushUniforms.mode = Terrainbrush::Raise;
                }
                ImGui::SameLine();

                if (drawModeButton("Lower", terrainEditorState.brushUniforms.mode, Terrainbrush::Lower))
                {
                    terrainEditorState.brushUniforms.mode = Terrainbrush::Lower;
                }
                ImGui::SameLine();

                if (drawModeButton("Flatten", terrainEditorState.brushUniforms.mode, Terrainbrush::Flatten))
                {
                    terrainEditorState.brushUniforms.mode = Terrainbrush::Flatten;
                }
                ImGui::SameLine();

                if (drawModeButton("Smooth", terrainEditorState.brushUniforms.mode, Terrainbrush::Smooth))
                {
                    terrainEditorState.brushUniforms.mode = Terrainbrush::Smooth;
                }

            }

            if (ImGui::CollapsingHeader("Brush"))
            {
                static Dynui::ImguiTextureId textureInfo;
                textureInfo.nebulaHandle = terrainEditorState.brushTexture;
                textureInfo.mip = 0;
                textureInfo.layer = 0;
                textureInfo.splat = 1;
                

                ImVec2 imageSize = { 64.0f, 64.0f };
                ImGui::Image((void*)&textureInfo, imageSize);
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    static const char* shapeStr = "Circle";
                    if (ImGui::BeginCombo("Shape", shapeStr))
                    {
                        if (ImGui::Selectable("Circle"))
                        {
                            shapeStr = "Circle";
                            terrainEditorState.brushShape = Terrainbrush::BrushGenerationShape::Circle;
                        }
                        if (ImGui::Selectable("Rectangle"))
                        {
                            shapeStr = "Rectangle";
                            terrainEditorState.brushShape = Terrainbrush::BrushGenerationShape::Rectangle;
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SliderFloat("Hardness", &terrainEditorState.brushHardness, 0.0f, 0.999f);
                    ImGui::SliderFloat("Falloff", &terrainEditorState.brushFalloff, 0.0f, 1.0f);
                    ImGui::SliderFloat("Size", &terrainEditorState.brushSize, 1.0f, 512.0f);
                    ImGui::SliderFloat("Strength", &terrainEditorState.brushUniforms.strength, 0.001f, 1.0f);
                }
                ImGui::EndGroup();


            }

            if (terrainEditorState.paint)
            {
                Terrain::TerrainContext::InvalidateTerrain(terrain.graphicsEntityId);
            }

            if (ImGui::CollapsingHeader("Biomes"))
            {
                if (ImGui::Button("New Biome"))
                {
                    Terrain::TerrainBiomeId biome = Terrain::TerrainContext::CreateBiome({});
                    terrainEditorState.biomes.Append(biome);
                    terrainEditorState.biomeSettings.Append({});
                    terrainEditorState.biomeTextures.Append(BiomeTextures());
                    BiomeTextures& textures = terrainEditorState.biomeTextures.Back();

                    // Create new mask
                    CoreGraphics::TextureCreateInfo maskTexInfo;
                    maskTexInfo.name = "Biome Mask Texture";
                    maskTexInfo.width = terrain.worldSizeX;
                    maskTexInfo.height = terrain.worldSizeZ;
                    maskTexInfo.format = CoreGraphics::PixelFormat::R8;
                    maskTexInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::ReadWrite;
                    textures.maskTex = CoreGraphics::CreateTexture(maskTexInfo);
                    textures.mask.nebulaHandle = textures.maskTex;
                    textures.mask.splat = 1;

                    terrainEditorState.activeBiomeMask = textures.maskTex;


                    Terrain::TerrainContext::SetBiomeMask(biome, textures.maskTex);

                    for (int i = 0; i < 4; i++)
                    {
                        Dynui::ImguiTextureId& alb = textures.albedo[i];
                        textures.albedoPaths[i] = "systex:white.dds";
                        textures.albedoResources[i] = Resources::CreateResource(textures.albedoPaths[i], "editor", nullptr, nullptr, true, false);
                        alb.nebulaHandle = textures.albedoResources[i];

                        Dynui::ImguiTextureId& nor = textures.normals[i];
                        textures.normalsPaths[i] = "systex:nobump.dds";
                        textures.normalsResources[i] = Resources::CreateResource(textures.normalsPaths[i], "editor", nullptr, nullptr, true, false);
                        nor.nebulaHandle = textures.normalsResources[i];

                        Dynui::ImguiTextureId& mat = textures.material[i];
                        textures.materialPaths[i] = "systex:default_material.dds";
                        textures.materialResources[i] = Resources::CreateResource(textures.materialPaths[i], "editor", nullptr, nullptr, true, false);
                        mat.nebulaHandle = textures.materialResources[i];
                    }
                    terrainEditorState.biomeNames.Append(Util::String::Sprintf("Biome %d", terrainEditorState.biomes.Size() - 1));
                }

                static int selectedBiome = -1;
                if (ImGui::BeginListBox("###BiomeList"))
                {
                    for (int i = 0; i < terrainEditorState.biomes.Size(); i++)
                    {
                        if (ImGui::Selectable(terrainEditorState.biomeNames[i].AsCharPtr()))
                        {
                            selectedBiome = i;
                        }
                    }
                    ImGui::EndListBox();
                }

                if (selectedBiome != -1)
                {

                    auto biomeId = terrainEditorState.biomes[selectedBiome];
                    auto& biomeSettings = terrainEditorState.biomeSettings[selectedBiome];
                    terrainEditorState.activeBiomeMask = terrainEditorState.biomeTextures[selectedBiome].maskTex;
                    CoreGraphics::ResourceTableSetRWTexture(terrainEditorState.brushResourceTable, {
                        terrainEditorState.activeBiomeMask, Terrainbrush::OutputMask::BINDING
                    });
                    CoreGraphics::ResourceTableCommitChanges(terrainEditorState.brushResourceTable);

                    ImGui::Text("Mask");
                    ImGui::Image(&terrainEditorState.biomeTextures[selectedBiome].mask, ImVec2(128, 128));
                    ImGui::SameLine();
                    if (drawModeButton("Paint", terrainEditorState.brushUniforms.mode, Terrainbrush::Biome))
                    {
                        terrainEditorState.brushUniforms.mode = Terrainbrush::Biome;
                    }

                    const char* labels[4] = { "Flat", "Slope", "Height", "Height slope" };
                    const char* textures[3] = { "Albedo", "Normals", "Material" };


                    for (int i = 0; i < 4; i++)
                    {
                        ImGui::BeginGroup();
                        ImGui::Text(labels[i]);
                        for (int j = 0; j < 3; j++)
                        {
                            bool pressed = false;

                            Dynui::ImguiTextureId* tex = nullptr;
                            const char* path = nullptr;
                            switch (j)
                            {
                            case 0:
                                tex = &terrainEditorState.biomeTextures[selectedBiome].albedo[i];
                                path = terrainEditorState.biomeTextures[selectedBiome].albedoPaths[i].Value();
                                break;
                            case 1:
                                tex = &terrainEditorState.biomeTextures[selectedBiome].normals[i];
                                path = terrainEditorState.biomeTextures[selectedBiome].normalsPaths[i].Value();
                                break;
                            case 2:
                                tex = &terrainEditorState.biomeTextures[selectedBiome].material[i];
                                path = terrainEditorState.biomeTextures[selectedBiome].materialPaths[i].Value();
                                break;
                            default:
                                n_error("Can't happen");
                                break;
                            }
                            pressed |= ImGui::ImageButton(textures[j], tex, ImVec2(64, 64));
                            ImGui::SameLine(); 
                            ImGui::BeginGroup();
                                ImGui::Text(textures[j]);
                                pressed |= ImGui::Button(path);
                            ImGui::EndGroup();

                            if (pressed)
                            {
                                // TODO: Replace file dialog with asset browser view/instance
                                const char* patterns[] = { "*.dds" };
                                const char* filePath = tinyfd_openFileDialog(textures[j], IO::URI(path).LocalPath().AsCharPtr(), 1, patterns, "Texture files (DDS)", false);

                                if (filePath != nullptr)
                                {
                                    switch (j)
                                    {
                                        case 0:
                                            terrainEditorState.biomeTextures[selectedBiome].albedoPaths[i] = filePath;
                                            terrainEditorState.biomeTextures[selectedBiome].albedoResources[i] = Resources::CreateResource(filePath, "terrain", nullptr, nullptr, true, false);
                                            terrainEditorState.biomeTextures[selectedBiome].albedo[i].nebulaHandle = terrainEditorState.biomeTextures[selectedBiome].albedoResources[i];
                                            break;
                                        case 1:
                                            terrainEditorState.biomeTextures[selectedBiome].normalsPaths[i] = filePath;
                                            terrainEditorState.biomeTextures[selectedBiome].normalsResources[i] = Resources::CreateResource(filePath, "terrain", nullptr, nullptr, true, false);
                                            terrainEditorState.biomeTextures[selectedBiome].normals[i].nebulaHandle = terrainEditorState.biomeTextures[selectedBiome].normalsResources[i];
                                            break;
                                        case 2:
                                            terrainEditorState.biomeTextures[selectedBiome].materialPaths[i] = filePath;
                                            terrainEditorState.biomeTextures[selectedBiome].materialResources[i] = Resources::CreateResource(filePath, "terrain", nullptr, nullptr, true, false);
                                            terrainEditorState.biomeTextures[selectedBiome].material[i].nebulaHandle = terrainEditorState.biomeTextures[selectedBiome].materialResources[i];
                                            break;
                                        default:
                                            n_error("Can't happen");
                                            break;
                                    }
                                }

                                // Update biome in terrain system
                                Terrain::TerrainContext::SetBiomeLayer(selectedBiome, Terrain::BiomeSettings::BiomeMaterialLayer(i),
                                    terrainEditorState.biomeTextures[selectedBiome].albedoPaths[i],
                                    terrainEditorState.biomeTextures[selectedBiome].normalsPaths[i],
                                    terrainEditorState.biomeTextures[selectedBiome].materialPaths[i]
                                );
                                Terrain::TerrainContext::InvalidateTerrain(terrain.graphicsEntityId);

                            }
                        }
                        ImGui::EndGroup();
                    }

                    bool applyRules = false;
                    ImGui::BeginGroup();
                        ImGui::Text("Rules");
                        applyRules |= ImGui::SliderFloat("Slope Threshold", &biomeSettings.biomeParameters.slopeThreshold, 0.0f, 1.0f);
                        applyRules |= ImGui::SliderFloat("Height Threshold", &biomeSettings.biomeParameters.heightThreshold, 0.0f, 1.0f);
                        applyRules |= ImGui::SliderFloat("UV Scale Factor", &biomeSettings.biomeParameters.uvScaleFactor, 1.0f, 256.0f);
                    ImGui::EndGroup();

                    if (applyRules)
                    {
                        Terrain::TerrainContext::SetBiomeRules(biomeId, biomeSettings.biomeParameters.slopeThreshold, biomeSettings.biomeParameters.heightThreshold, biomeSettings.biomeParameters.uvScaleFactor);
                        Terrain::TerrainContext::InvalidateTerrain(terrain.graphicsEntityId);
                    }
                }
            }
        }
    }
}

} // namespace Presentation

namespace Tools
{

//------------------------------------------------------------------------------
/**
*/
void
TerrainEditorTool::Render(Presentation::Modules::Viewport* viewport)
{

    terrainEditorState.drawBrushPreview = false;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainEditorTool::Update(Presentation::Modules::Viewport* viewport)
{
    Ptr<Input::Mouse> mouse = Input::InputServer::Instance()->GetDefaultMouse();
    Math::vec2 mousePos = mouse->GetPixelPosition();
    terrainEditorState.brushUniforms.cursor[0] = mousePos.x - viewport->lastViewportImagePositionAbsolute.x;
    terrainEditorState.brushUniforms.cursor[1] = mousePos.y - viewport->lastViewportImagePositionAbsolute.y;
    CoreGraphics::BufferUpdate(terrainEditorState.brushUniformBuffer, terrainEditorState.brushUniforms);
    terrainEditorState.drawBrushPreview = true;
}

//------------------------------------------------------------------------------
/**
*/
bool 
TerrainEditorTool::IsModifying() const
{
    return terrainEditorState.paint;
}

}