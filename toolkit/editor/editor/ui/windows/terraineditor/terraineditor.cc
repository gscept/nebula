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

#include "gpulang/editor/editor/ui/windows/terraineditor/terrainbrush.h"

using namespace Editor;


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
    CoreGraphics::ShaderProgramId brushShaderProgram;

    CoreGraphics::ShaderProgramId brushPreviewProgram;
    CoreGraphics::PipelineId brushPreviewPipeline;

    Resources::ResourceId brushMeshResource;
    CoreGraphics::MeshResourceId brushMesh;

    CoreGraphics::ShaderProgramId brushGenerationProgram;
    CoreGraphics::TextureId brushTexture;
    Terrainbrush::BrushGenerationUniforms::STRUCT brushGenerationUniforms;
    CoreGraphics::BufferId brushGenerationUniformBuffer;
} terrainEditorState;

namespace Presentation
{
__ImplementClass(Presentation::TerrainEditor, 'TrEd', Presentation::BaseWindow);

static CoreGraphics::TextureId heightmapTex = CoreGraphics::InvalidTextureId;
//------------------------------------------------------------------------------
/**
*/
TerrainEditor::TerrainEditor()
{
    terrainEditorState.brushShader = CoreGraphics::ShaderGet("shd:editor/ui/windows/terraineditor/terrainbrush.gplb");
    terrainEditorState.brushShaderProgram = CoreGraphics::ShaderGetProgram(terrainEditorState.brushShader, CoreGraphics::ShaderFeatureMask("Brush"));
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
            CoreGraphics::CmdSetShaderProgram(cmdBuf, terrainEditorState.brushShaderProgram);
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

    Terrain::TerrainContext::CreateBiome({});
}

//------------------------------------------------------------------------------
/**
*/
TerrainEditor::~TerrainEditor()
{
    // empty
}

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
            if (heightmapTex != CoreGraphics::InvalidTextureId)
            {
                static Dynui::ImguiTextureId textureInfo;
                textureInfo.nebulaHandle = heightmapTex;
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
                if (heightmapTex != CoreGraphics::InvalidTextureId)
                {
                    CoreGraphics::DestroyTexture(heightmapTex);
                    heightmapTex = CoreGraphics::InvalidTextureId;
                }

                CoreGraphics::TextureCreateInfo texInfo;
                texInfo.name = "Editor Height Map";
                texInfo.width = terrain.worldSizeX;
                texInfo.height = terrain.worldSizeZ;
                texInfo.format = CoreGraphics::PixelFormat::R16F;
                texInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::ReadWrite | CoreGraphics::TextureUsage::TransferSource;
                heightmapTex = CoreGraphics::CreateTexture(texInfo);
                Terrain::TerrainContext::SetHeightmap(terrain.graphicsEntityId, heightmapTex);

                terrainEditorState.brushUniforms.heightmapSize[0] = terrain.worldSizeX;
                terrainEditorState.brushUniforms.heightmapSize[1] = terrain.worldSizeZ;

                CoreGraphics::ResourceTableSetRWTexture(terrainEditorState.brushResourceTable, {
                    heightmapTex, Terrainbrush::Output::BINDING
                });

                CoreGraphics::ResourceTableCommitChanges(terrainEditorState.brushResourceTable);
            }

            if (heightmapTex != CoreGraphics::InvalidTextureId)
            {

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
void
TerrainEditorTool::Update(Presentation::Modules::Viewport* viewport)
{
    terrainEditorState.drawBrushPreview = false;
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