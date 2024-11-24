//------------------------------------------------------------------------------
//  imguirenderer.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "imguicontext.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "graphics/graphicsserver.h"
#include "resources/resourceserver.h"
#include "math/rectangle.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/graphicsdevice.h"
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "frame/framesubgraph.h"
#include "core/cvar.h"
#include "appgame/gameapplication.h"

#include "frame/default.h"
#if WITH_NEBULA_EDITOR
#include "frame/editorframe.h"
#endif

using namespace Math;
using namespace CoreGraphics;
using namespace Base;
using namespace Input;

namespace Dynui
{

ImguiContext::ImguiState ImguiContext::state;
static Core::CVar* ui_opacity;

//------------------------------------------------------------------------------
/**
    Imgui rendering function
*/
void
ImguiDrawFunction(const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport)
{
    ImDrawData* data = ImGui::GetDrawData();
    // get Imgui context
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(viewport.width() * io.DisplayFramebufferScale.x);
    int fb_height = (int)(viewport.height() * io.DisplayFramebufferScale.y);
    data->ScaleClipRects(io.DisplayFramebufferScale);

    // get renderer
    //const Ptr<BufferLock>& vboLock = renderer->GetVertexBufferLock();
    //const Ptr<BufferLock>& iboLock = renderer->GetIndexBufferLock();
    IndexT currentBuffer = CoreGraphics::GetBufferedFrameIndex();
    BufferId vbo = ImguiContext::state.vbos[currentBuffer];
    BufferId ibo = ImguiContext::state.ibos[currentBuffer];

    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_GRAPHICS, "ImGUI");

    // apply shader
    //CoreGraphics::CmdSetShaderProgram(cmdBuf, state.prog);

    // create orthogonal matrix
#if __VULKAN__
    mat4 proj = orthooffcenterrh(0.0f, viewport.width(), viewport.height(), 0.0f, -1.0f, +1.0f);
#else
    mat4 proj = orthooffcenterrh(0.0f, viewport.width(), 0.0f, viewport.height(), -1.0f, +1.0f);
#endif

    // if buffers are too small, create new buffers
    if (data->TotalVtxCount > CoreGraphics::BufferGetSize(ImguiContext::state.vbos[currentBuffer]))
    {
        CoreGraphics::BufferUnmap(ImguiContext::state.vbos[currentBuffer]);
        CoreGraphics::DestroyBuffer(ImguiContext::state.vbos[currentBuffer]);

        CoreGraphics::BufferCreateInfo vboInfo;
        vboInfo.name = "ImGUI VBO"_atm;
        vboInfo.size = Math::roundtopow2(data->TotalVtxCount);
        vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(ImguiContext::state.vlo);
        vboInfo.mode = CoreGraphics::HostCached;
        vboInfo.usageFlags = CoreGraphics::VertexBuffer;
        vboInfo.data = nullptr;
        vboInfo.dataSize = 0;
        ImguiContext::state.vbos[currentBuffer] = CoreGraphics::CreateBuffer(vboInfo);
        ImguiContext::state.vertexPtrs[currentBuffer] = (byte*)CoreGraphics::BufferMap(ImguiContext::state.vbos[currentBuffer]);
    }

    if (data->TotalIdxCount > CoreGraphics::BufferGetSize(ImguiContext::state.ibos[currentBuffer]))
    {
        CoreGraphics::BufferUnmap(ImguiContext::state.ibos[currentBuffer]);
        CoreGraphics::DestroyBuffer(ImguiContext::state.ibos[currentBuffer]);

        CoreGraphics::BufferCreateInfo iboInfo;
        iboInfo.name = "ImGUI IBO"_atm;
        iboInfo.size = Math::roundtopow2(data->TotalIdxCount);
        iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(IndexType::Index16);
        iboInfo.mode = CoreGraphics::HostCached;
        iboInfo.usageFlags = CoreGraphics::IndexBuffer;
        iboInfo.data = nullptr;
        iboInfo.dataSize = 0;
        ImguiContext::state.ibos[currentBuffer] = CoreGraphics::CreateBuffer(iboInfo);
        ImguiContext::state.indexPtrs[currentBuffer] = (byte*)CoreGraphics::BufferMap(ImguiContext::state.ibos[currentBuffer]);
    }

    // setup device
#if WITH_NEBULA_EDITOR
    if (App::GameApplication::IsEditorEnabled())
    {
        CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, ImguiContext::state.editorPipeline);
    }
    else
    {
        CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, ImguiContext::state.pipeline);
    }
#else
    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, ImguiContext::state.pipeline);
#endif

    // setup input buffers
    CoreGraphics::CmdSetVertexLayout(cmdBuf, ImguiContext::state.vlo);
    CoreGraphics::CmdSetResourceTable(cmdBuf, ImguiContext::state.resourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);
    CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, ImguiContext::state.vbos[currentBuffer], 0);
    CoreGraphics::CmdSetIndexBuffer(cmdBuf, IndexType::Index16, ImguiContext::state.ibos[currentBuffer], 0);
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::CmdSetViewport(cmdBuf, viewport, 0);
    CoreGraphics::CmdSetScissorRect(cmdBuf, viewport, 0);

    // set projection
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, ImguiContext::state.textProjectionConstant, sizeof(proj), (byte*)&proj);

    struct TextureInfo
    {
        uint32 type : 4;
        uint32 layer : 8;
        uint32 mip : 8;
        uint32 useAlpha : 1;
        uint32 id : 11;
    };

    IndexT vertexOffset = 0;
    IndexT indexOffset = 0;
    IndexT vertexBufferOffset = 0;
    IndexT indexBufferOffset = 0;

    IndexT i;
    for (i = 0; i < data->CmdListsCount; i++)
    {
        // get buffer data and calculate size of data to copy
        const ImDrawList* commandList = data->CmdLists[i];
        const unsigned char* vertexBuffer = (unsigned char*)&commandList->VtxBuffer.front();
        const unsigned char* indexBuffer = (unsigned char*)&commandList->IdxBuffer.front();
        const SizeT vertexBufferSize = commandList->VtxBuffer.size() * sizeof(ImDrawVert);                  // 2 for position, 2 for uvs, 1 int for color
        const SizeT indexBufferSize = commandList->IdxBuffer.size() * sizeof(ImDrawIdx);                    // using 16 bit indices

        // if we render too many vertices, we will simply assert, but should never happen really
        n_assert(vertexBufferOffset + (IndexT)commandList->VtxBuffer.size() < CoreGraphics::BufferGetByteSize(ImguiContext::state.vbos[currentBuffer]));
        n_assert(indexBufferOffset + (IndexT)commandList->IdxBuffer.size() < CoreGraphics::BufferGetByteSize(ImguiContext::state.ibos[currentBuffer]));

        // wait for previous draws to finish...
        Memory::Copy(vertexBuffer, ImguiContext::state.vertexPtrs[currentBuffer] + vertexBufferOffset, vertexBufferSize);
        Memory::Copy(indexBuffer, ImguiContext::state.indexPtrs[currentBuffer] + indexBufferOffset, indexBufferSize);
        IndexT j;
        IndexT primitiveIndexOffset = 0;
        for (j = 0; j < commandList->CmdBuffer.size(); j++)
        {
            const ImDrawCmd* command = &commandList->CmdBuffer[j];
            if (command->UserCallback)
            {
                command->UserCallback(commandList, command);
            }
            else
            {
                // setup scissor rect
                Math::rectangle<int> scissorRect((int)command->ClipRect.x, (int)command->ClipRect.y, (int)command->ClipRect.z, (int)command->ClipRect.w);
                CoreGraphics::CmdSetScissorRect(cmdBuf, scissorRect, 0);
                ImguiTextureId tex = *(ImguiTextureId*)command->TextureId;

                TextureInfo texInfo;
                texInfo.type = 0;
                texInfo.useAlpha = tex.useAlpha;

                // set texture in shader, we shouldn't have to put it into ImGui
                CoreGraphics::TextureId texture = tex.nebulaHandle;
                CoreGraphics::TextureIdLock _0(texture);
                CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(texture);
                auto usage = CoreGraphics::TextureGetUsage(texture);
                if (usage & CoreGraphics::TextureUsage::RenderTexture || usage & CoreGraphics::TextureUsage::ReadWriteTexture)
                {
                    texInfo.useAlpha = false;
                }
                SizeT layers = CoreGraphics::TextureGetNumLayers(texture);
                if (layers > 1)
                {
                    texInfo.type = 1;
                }
                if (dims.depth > 1)
                {
                    texInfo.type = 2;
                }
                texInfo.layer = tex.layer;
                texInfo.mip = tex.mip;
                texInfo.id = CoreGraphics::TextureGetBindlessHandle(texture);

                CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, ImguiContext::state.packedTextureInfo, sizeof(TextureInfo), (byte*)& texInfo);

                // setup primitive
                CoreGraphics::PrimitiveGroup primitive;
                primitive.SetNumIndices(command->ElemCount);
                primitive.SetBaseIndex(primitiveIndexOffset + indexOffset);
                primitive.SetBaseVertex(vertexOffset);

                // prepare render device and draw
                CoreGraphics::CmdDraw(cmdBuf, primitive);
            }

            // increment vertex offset
            primitiveIndexOffset += command->ElemCount;
        }

        // bump vertices
        vertexOffset += commandList->VtxBuffer.size();
        indexOffset += commandList->IdxBuffer.size();

        // lock buffers
        vertexBufferOffset += vertexBufferSize;
        indexBufferOffset += indexBufferSize;
    }

    CoreGraphics::BufferFlush(ImguiContext::state.vbos[currentBuffer]);
    CoreGraphics::BufferFlush(ImguiContext::state.ibos[currentBuffer]);
}

//------------------------------------------------------------------------------
/**
    @todo   update imgui and uncomment the commented code below!
*/
void
ImguiContext::RecoverImGuiContextErrors()
{
    ImGuiContext& g = *ImGui::GetCurrentContext();

    while (g.CurrentWindowStack.Size > 0)
    {
#ifdef IMGUI_HAS_TABLE
        while (g.CurrentTable && (g.CurrentTable->OuterWindow == g.CurrentWindow || g.CurrentTable->InnerWindow == g.CurrentWindow))
        {
            n_warning("Recovered from missing EndTable() call.");
            ImGui::EndTable();
        }
#endif

        while (g.CurrentTabBar != NULL)
        {
            n_warning("WARNING: Recovered from missing ImGui::EndTabBar() call.\n");
            ImGui::EndTabBar();
        }

        while (g.CurrentWindow->DC.TreeDepth > 0)
        {
            n_warning("WARNING: Recovered from missing ImGui::TreePop() call.\n");
            ImGui::TreePop();
        }

        //while (g.GroupStack.Size > g.CurrentWindow->DC.StackSizesOnBegin.SizeOfGroupStack)
        //{
        //  n_warning("WARNING: Recovered from missing ImGui::EndGroup() call.\n");
        //  ImGui::EndGroup();
        //}

        while (g.CurrentWindow->IDStack.Size > 1)
        {
            n_warning("WARNING: Recovered from missing ImGui::PopID() call.\n");
            ImGui::PopID();
        }

        //while (g.ColorStack.Size > g.CurrentWindow->DC.StackSizesOnBegin.SizeOfColorStack)
        //{
        //  n_warning("WARNING: Recovered from missing PopStyleColor() for '%s'\n", ImGui::GetStyleColorName(g.ColorStack.back().Col));
        //  ImGui::PopStyleColor();
        //}
        //while (g.StyleVarStack.Size > g.CurrentWindow->DC.StackSizesOnBegin.SizeOfStyleVarStack)
        //{
        //  n_warning("WARNING: Recovered from missing ImGui::PopStyleVar() call.\n");
        //  ImGui::PopStyleVar();
        //}
        //while (g.FocusScopeStack.Size > g.CurrentWindow->DC.StackSizesOnBegin.SizeOfFocusScopeStack)
        //{
        //  n_warning("WARNING: Recovered from missing ImGui::PopFocusScope() call.\n");
        //  ImGui::PopFocusScope();
        //}

        if (g.CurrentWindowStack.Size == 1)
        {
            break;
        }

        if (g.CurrentWindow->Flags & ImGuiWindowFlags_ChildWindow)
        {
            n_warning("WARNING: Recovered from missing ImGui::EndChild() call.\n");
            ImGui::EndChild();
        }
        else
        {
            n_warning("WARNING: Recovered from missing ImGui::End() call.\n");
            ImGui::End();
        }
    }
}

__ImplementPluginContext(ImguiContext);
//------------------------------------------------------------------------------
/**
*/
ImguiContext::ImguiContext()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ImguiContext::~ImguiContext()
{

}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::Create()
{
    ui_opacity = Core::CVarCreate(Core::CVar_Float, "ui_opacity", "1.0", "Global UI opacity (0..1)");

    __bundle.OnWindowResized = ImguiContext::OnWindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    state.dockOverViewport = false;

    // allocate imgui shader
    state.uiShader = CoreGraphics::ShaderGet("shd:imgui/shaders/imgui.fxb");
    state.prog = CoreGraphics::ShaderGetProgram(state.uiShader, CoreGraphics::ShaderFeatureMask("Static"));

    state.resourceTable = CoreGraphics::ShaderCreateResourceTable(state.uiShader, NEBULA_BATCH_GROUP);

    state.textProjectionConstant = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "TextProjectionModel");
    state.packedTextureInfo = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "PackedTextureInfo");

    state.inputHandler = ImguiInputHandler::Create();
    Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, state.inputHandler.upcast<Input::InputHandler>());

    // create vertex buffer
    Util::Array<CoreGraphics::VertexComponent> components;
    components.Append(VertexComponent(0, VertexComponent::Float2, 0));
    components.Append(VertexComponent(1, VertexComponent::Float2, 0));
    components.Append(VertexComponent(2, VertexComponent::UByte4N, 0));
    state.vlo = CoreGraphics::CreateVertexLayout({ .name = "ImGui"_atm, .comps = components });

    /*
    FrameScript_default::RegisterSubgraphPipelines_ImGUI_Pass([](const CoreGraphics::PassId pass, uint subpass)
    {
        CoreGraphics::InputAssemblyKey inputAssembly{ CoreGraphics::PrimitiveTopology::TriangleList, false };
        if (state.pipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(state.pipeline);
        state.pipeline = CoreGraphics::CreateGraphicsPipeline({ state.prog, pass, subpass, inputAssembly });
    });
    FrameScript_default::RegisterSubgraph_ImGUI_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
#ifdef NEBULA_NO_DYNUI_ASSERTS
        ImguiContext::RecoverImGuiContextErrors();
#endif
        ImGui::Render();
        ImguiDrawFunction(cmdBuf, viewport);
    });
    */

#if WITH_NEBULA_EDITOR
    if (App::GameApplication::IsEditorEnabled())
    {
        FrameScript_editorframe::RegisterSubgraphPipelines_ImGUI_Pass([](const CoreGraphics::PassId pass, uint subpass)
            {
                CoreGraphics::InputAssemblyKey inputAssembly{ CoreGraphics::PrimitiveTopology::TriangleList, false };
                if (state.editorPipeline != CoreGraphics::InvalidPipelineId)
                    CoreGraphics::DestroyGraphicsPipeline(state.editorPipeline);
                state.editorPipeline = CoreGraphics::CreateGraphicsPipeline({ state.prog, pass, subpass, inputAssembly });
            });

        FrameScript_editorframe::RegisterSubgraph_ImGUI_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
            {
#ifdef NEBULA_NO_DYNUI_ASSERTS
                ImguiContext::RecoverImGuiContextErrors();
#endif
                ImGui::Render();
                ImguiDrawFunction(cmdBuf, viewport, true);
            });
    }
#endif

    SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "ImGUI VBO"_atm;
    vboInfo.size = 1;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(state.vlo);
    vboInfo.mode = CoreGraphics::HostCached;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.data = nullptr;
    vboInfo.dataSize = 0;
    state.vbos.Resize(numBuffers);
    IndexT i;
    for (i = 0; i < numBuffers; i++)
    {
        state.vbos[i] = CoreGraphics::CreateBuffer(vboInfo);
    }

    CoreGraphics::BufferCreateInfo iboInfo;
    iboInfo.name = "ImGUI IBO"_atm;
    iboInfo.size = 1;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(IndexType::Index16);
    iboInfo.mode = CoreGraphics::HostCached;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = nullptr;
    iboInfo.dataSize = 0;
    state.ibos.Resize(numBuffers);
    for (i = 0; i < numBuffers; i++)
    {
        iboInfo.name = Util::String::Sprintf("imgui_ibo_%d", i);
        state.ibos[i] = CoreGraphics::CreateBuffer(iboInfo);
    }

    // map buffer
    state.vertexPtrs.Resize(numBuffers);
    state.indexPtrs.Resize(numBuffers);
    for (i = 0; i < numBuffers; i++)
    {
        state.vertexPtrs[i] = (byte*)CoreGraphics::BufferMap(state.vbos[i]);
        state.indexPtrs[i] = (byte*)CoreGraphics::BufferMap(state.ibos[i]);
    }

    // get display mode, this will be our default size
    DisplayMode mode = CoreGraphics::WindowGetDisplayMode(CurrentWindow);

    float scaleFactor = mode.GetContentScale();
    // setup Imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)mode.GetWidth(), (float)mode.GetHeight());
    io.DeltaTime = 1 / 60.0f;
    //io.PixelCenterOffset = 0.0f;
    //io.FontTexUvForWhite = ImVec2(1, 1);
    //io.RenderDrawListsFn = ImguiDrawFunction;

    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.WindowRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.TabMinWidthForCloseButton = FLT_MAX;
    style.WindowTitleAlign = { 0.0f, 0.52f };
    style.WindowMenuButtonPosition = ImGuiDir_Right;

    style.WindowPadding = { 10.0f, 10.0f };
    // FIXME: ImGui seems to have problems with the "X" (close window) button when setting framepadding to anything higher than ~4. Could be the docking branch which is currently in beta.
    style.FramePadding = { 8, 3 };//{ 16, 3 };
    style.ItemInnerSpacing = { 2, 2 };
    style.ItemSpacing = { 2, 2 };
    style.IndentSpacing = 10.0f;
    style.GrabMinSize = 8.0f;

    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.ChildBorderSize = 0.0f;

    style.ScaleAllSizes(scaleFactor);


    ImGui::GetStyle().Alpha = Core::CVarReadFloat(ui_opacity);
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 0.50f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.59f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.17f, 0.17f, 0.17f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.28f, 0.28f, 0.28f, 0.25f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.22f, 0.22f, 0.22f, 0.89f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.29f, 0.29f, 0.29f, 0.07f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.90f, 0.37f, 0.00f, 0.38f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.38f, 0.00f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.30f, 0.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.31f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.25f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.30f, 0.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(1.00f, 0.55f, 0.10f, 0.60f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.55f, 0.10f, 0.70f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.74f, 0.37f, 0.00f, 0.90f);
    colors[ImGuiCol_Separator] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(1.00f, 0.30f, 0.00f, 0.70f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.90f, 0.20f, 0.05f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 0.50f, 0.30f, 0.11f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.82f, 0.33f, 0.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.50f, 0.40f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.04f, 0.04f, 0.04f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.16f, 0.16f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.71f, 0.27f, 0.00f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(1.00f, 0.30f, 0.00f, 0.23f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.039f, 0.039f, 0.039f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.174f, 0.174f, 0.174f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.022f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.0f);

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = Key::Tab;
    io.KeyMap[ImGuiKey_LeftArrow] = Key::Left;
    io.KeyMap[ImGuiKey_RightArrow] = Key::Right;
    io.KeyMap[ImGuiKey_UpArrow] = Key::Up;
    io.KeyMap[ImGuiKey_DownArrow] = Key::Down;
    io.KeyMap[ImGuiKey_Home] = Key::Home;
    io.KeyMap[ImGuiKey_End] = Key::End;
    io.KeyMap[ImGuiKey_Delete] = Key::Delete;
    io.KeyMap[ImGuiKey_Backspace] = Key::Back;
    io.KeyMap[ImGuiKey_Enter] = Key::Return;
    io.KeyMap[ImGuiKey_Escape] = Key::Escape;
    io.KeyMap[ImGuiKey_Space] = Key::Space;
    io.KeyMap[ImGuiKey_A] = Key::A;
    io.KeyMap[ImGuiKey_C] = Key::C;
    io.KeyMap[ImGuiKey_V] = Key::V;
    io.KeyMap[ImGuiKey_X] = Key::X;
    io.KeyMap[ImGuiKey_Y] = Key::Y;
    io.KeyMap[ImGuiKey_Z] = Key::Z;

    // enable keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

    // load default font
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 1;
#if __WIN32__
    state.normalFont = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/calibri.ttf", scaleFactor * 14, &config);
    state.smallFont = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/calibri.ttf", scaleFactor * 12, &config);
    state.boldFont = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/calibrib.ttf", scaleFactor * 14, &config);
    state.itFont = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/calibrii.ttf", scaleFactor * 14, &config);
#else
    state.normalFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSans.ttf", scaleFactor * 14, &config);
    state.smallFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSans.ttf", scaleFactor * 12, &config);
    state.boldFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf", scaleFactor * 12, &config);
    state.itFont = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSansItalic.ttf", scaleFactor * 12, &config);
#endif

    unsigned char* buffer;
    int width, height, bytesPerPixel;
    io.Fonts->GetTexDataAsRGBA32(&buffer, &width, &height, &bytesPerPixel);

    // load image using SOIL
    // unsigned char* texData = SOIL_load_image_from_memory(buffer, width * height * bytesPerPixel, &width, &height, &bytesPerPixel, SOIL_LOAD_AUTO);

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "imgui_font_tex"_atm;
    texInfo.usage = TextureUsage::SampleTexture;
    texInfo.tag = "system"_atm;
    texInfo.data = buffer;
    texInfo.dataSize = width * height * bytesPerPixel;
    texInfo.type = TextureType::Texture2D;
    texInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    texInfo.width = width;
    texInfo.height = height;

    state.fontTexture.nebulaHandle = CoreGraphics::CreateTexture(texInfo);
    state.fontTexture.mip = 0;
    state.fontTexture.layer = 0;
    state.fontTexture.useAlpha = true;
    io.Fonts->TexID = &state.fontTexture;
    io.Fonts->ClearTexData();

    if (!App::GameApplication::IsEditorEnabled())
    {
        // load settings from disk. If we don't do this here we need to
        // run an entire frame before being able to create or load settings
        if (!IO::IoServer::Instance()->FileExists("imgui.ini"))
        {
            ImGui::SaveIniSettingsToDisk("imgui.ini");
        }
        ImGui::LoadIniSettingsFromDisk("imgui.ini");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::Discard()
{
    IndexT i;
    for (i = 0; i < state.vbos.Size(); i++)
    {
        CoreGraphics::BufferUnmap(state.vbos[i]);
        CoreGraphics::BufferUnmap(state.ibos[i]);

        CoreGraphics::DestroyBuffer(state.vbos[i]);
        CoreGraphics::DestroyBuffer(state.ibos[i]);

        state.vertexPtrs[i] = nullptr;
        state.indexPtrs[i] = nullptr;
    }

    Input::InputServer::Instance()->RemoveInputHandler(state.inputHandler.upcast<InputHandler>());
    state.inputHandler = nullptr;

    CoreGraphics::DestroyTexture((CoreGraphics::TextureId)state.fontTexture.nebulaHandle);
    ImGui::DestroyContext();
}


const ImGuiKey NebulaToImguiKeyCodes[] =
{
    ImGuiKey_Backspace,      // Code::Back
    ImGuiKey_Tab,            // Code::Tab
    ImGuiKey_None,           // Code::Clear (No direct mapping)
    ImGuiKey_Enter,          // Code::Return
    ImGuiKey_LeftShift,      // Code::Shift (Assuming Left Shift)
    ImGuiKey_LeftCtrl,       // Code::Control (Assuming Left Control)
    ImGuiKey_Menu,           // Code::Menu
    ImGuiKey_Pause,          // Code::Pause
    ImGuiKey_CapsLock,       // Code::Capital
    ImGuiKey_Escape,         // Code::Escape
    ImGuiKey_None,           // Code::Convert (No direct mapping)
    ImGuiKey_None,           // Code::NonConvert (No direct mapping)
    ImGuiKey_None,           // Code::Accept (No direct mapping)
    ImGuiKey_None,           // Code::ModeChange (No direct mapping)
    ImGuiKey_Space,          // Code::Space
    ImGuiKey_PageUp,         // Code::Prior
    ImGuiKey_PageDown,       // Code::Next
    ImGuiKey_End,            // Code::End
    ImGuiKey_Home,           // Code::Home
    ImGuiKey_LeftArrow,      // Code::Left
    ImGuiKey_RightArrow,     // Code::Right
    ImGuiKey_UpArrow,        // Code::Up
    ImGuiKey_DownArrow,      // Code::Down
    ImGuiKey_None,           // Code::Select (No direct mapping)
    ImGuiKey_PrintScreen,    // Code::Print
    ImGuiKey_None,           // Code::Execute (No direct mapping)
    ImGuiKey_PrintScreen,    // Code::Snapshot
    ImGuiKey_Insert,         // Code::Insert
    ImGuiKey_Delete,         // Code::Delete
    ImGuiKey_None,           // Code::Help (No direct mapping)
    ImGuiKey_None,           // Code::LeftWindows (No direct mapping)
    ImGuiKey_None,           // Code::RightWindows (No direct mapping)
    ImGuiKey_None,           // Code::Apps (No direct mapping)
    ImGuiKey_None,           // Code::Sleep (No direct mapping)
    ImGuiKey_Keypad0,        // Code::NumPad0
    ImGuiKey_Keypad1,        // Code::NumPad1
    ImGuiKey_Keypad2,        // Code::NumPad2
    ImGuiKey_Keypad3,        // Code::NumPad3
    ImGuiKey_Keypad4,        // Code::NumPad4
    ImGuiKey_Keypad5,        // Code::NumPad5
    ImGuiKey_Keypad6,        // Code::NumPad6
    ImGuiKey_Keypad7,        // Code::NumPad7
    ImGuiKey_Keypad8,        // Code::NumPad8
    ImGuiKey_Keypad9,        // Code::NumPad9
    ImGuiKey_KeypadMultiply, // Code::Multiply
    ImGuiKey_KeypadAdd,      // Code::Add
    ImGuiKey_KeypadSubtract, // Code::Subtract
    ImGuiKey_None,           // Code::Separator (No direct mapping)
    ImGuiKey_KeypadDecimal,  // Code::Decimal
    ImGuiKey_KeypadDivide,   // Code::Divide
    ImGuiKey_F1,             // Code::F1
    ImGuiKey_F2,             // Code::F2
    ImGuiKey_F3,             // Code::F3
    ImGuiKey_F4,             // Code::F4
    ImGuiKey_F5,             // Code::F5
    ImGuiKey_F6,             // Code::F6
    ImGuiKey_F7,             // Code::F7
    ImGuiKey_F8,             // Code::F8
    ImGuiKey_F9,             // Code::F9
    ImGuiKey_F10,            // Code::F10
    ImGuiKey_F11,            // Code::F11
    ImGuiKey_F12,            // Code::F12
    ImGuiKey_NumLock,        // Code::NumLock
    ImGuiKey_ScrollLock,     // Code::Scroll
    ImGuiKey_Semicolon,      // Code::Semicolon
    ImGuiKey_Slash,          // Code::Slash
    ImGuiKey_GraveAccent,    // Code::Tilde
    ImGuiKey_LeftBracket,    // Code::LeftBracket
    ImGuiKey_RightBracket,   // Code::RightBracket
    ImGuiKey_Backslash,      // Code::BackSlash
    ImGuiKey_Apostrophe,     // Code::Quote
    ImGuiKey_Comma,          // Code::Comma
    ImGuiKey_None,           // Code::Underbar (No direct mapping)
    ImGuiKey_Period,         // Code::Period
    ImGuiKey_Equal,          // Code::Equality
    ImGuiKey_LeftShift,      // Code::LeftShift
    ImGuiKey_RightShift,     // Code::RightShift
    ImGuiKey_LeftCtrl,       // Code::LeftControl
    ImGuiKey_RightCtrl,      // Code::RightControl
    ImGuiKey_LeftAlt,        // Code::LeftMenu (assuming this maps to Alt)
    ImGuiKey_RightAlt,       // Code::RightMenu
    ImGuiKey_None,           // Code::BrowserBack (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::BrowserForward (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::BrowserRefresh (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::BrowserStop (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::BrowserSearch (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::BrowserFavorites (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::BrowserHome (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::VolumeMute (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::VolumeDown (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::VolumeUp (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::MediaNextTrack (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::MediaPrevTrack (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::MediaStop (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::MediaPlayPause (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::LaunchMail (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::LaunchMediaSelect (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::LaunchApp1 (No direct mapping in ImGuiKey)
    ImGuiKey_None,           // Code::LaunchApp2 (No direct mapping in ImGuiKey)
    ImGuiKey_0,              // Code::Key0
    ImGuiKey_1,              // Code::Key1
    ImGuiKey_2,              // Code::Key2
    ImGuiKey_3,              // Code::Key3
    ImGuiKey_4,              // Code::Key4
    ImGuiKey_5,              // Code::Key5
    ImGuiKey_6,              // Code::Key6
    ImGuiKey_7,              // Code::Key7
    ImGuiKey_8,              // Code::Key8
    ImGuiKey_9,              // Code::Key9
    ImGuiKey_A,              // Code::A
    ImGuiKey_B,              // Code::B
    ImGuiKey_C,              // Code::C
    ImGuiKey_D,              // Code::D
    ImGuiKey_E,              // Code::E
    ImGuiKey_F,              // Code::F
    ImGuiKey_G,              // Code::G
    ImGuiKey_H,              // Code::H
    ImGuiKey_I,              // Code::I
    ImGuiKey_J,              // Code::J
    ImGuiKey_K,              // Code::K
    ImGuiKey_L,              // Code::L
    ImGuiKey_M,              // Code::M
    ImGuiKey_N,              // Code::N
    ImGuiKey_O,              // Code::O
    ImGuiKey_P,              // Code::P
    ImGuiKey_Q,              // Code::Q
    ImGuiKey_R,              // Code::R
    ImGuiKey_S,              // Code::S
    ImGuiKey_T,              // Code::T
    ImGuiKey_U,              // Code::U
    ImGuiKey_V,              // Code::V
    ImGuiKey_W,              // Code::W
    ImGuiKey_X,              // Code::X
    ImGuiKey_Y,              // Code::Y
    ImGuiKey_Z,              // Code::Z
    ImGuiKey_None,           // Code::NumKeyCodes (No direct mapping, represents total count)
    ImGuiKey_None            // Code::InvalidKey (No direct mapping)
};

static bool KeysToRelease[ImGuiKey_COUNT] = { false };

//------------------------------------------------------------------------------
/**
*/
bool
ImguiContext::HandleInput(const Input::InputEvent& event)
{
    ImGuiIO& io = ImGui::GetIO();
    switch (event.GetType())
    {
    case InputEvent::KeyDown:
        io.KeysDown[event.GetKey()] = true;
        if (event.GetKey() == Key::LeftControl || event.GetKey() == Key::RightControl) io.KeyCtrl = true;
        if (event.GetKey() == Key::LeftShift || event.GetKey() == Key::RightShift) io.KeyShift = true;
        return io.WantCaptureKeyboard;
    case InputEvent::KeyUp:
        KeysToRelease[event.GetKey()] = true;
        if (event.GetKey() == Key::LeftControl || event.GetKey() == Key::RightControl) io.KeyCtrl = false;
        if (event.GetKey() == Key::LeftShift || event.GetKey() == Key::RightShift) io.KeyShift = false;
        return io.WantCaptureKeyboard;                                  // not a bug, this allows keys to be let go even if we are over the UI
    case InputEvent::Character:
    {
        char c = event.GetChar();

        // ignore backspace as a character
        if (c > 0 && c < 0x10000)
        {
            const char chars[] = { c, '\0' };
            io.AddInputCharactersUTF8(chars);
        }
        return io.WantTextInput;
    }
    case InputEvent::MouseMove:
        io.MousePos = ImVec2(event.GetAbsMousePos().x, event.GetAbsMousePos().y);
        return io.WantCaptureMouse;
    case InputEvent::MouseButtonDown:
        io.MouseDown[event.GetMouseButton()] = true;
        return io.WantCaptureMouse;
    case InputEvent::MouseButtonUp:
        io.MouseDown[event.GetMouseButton()] = false;
        return false;                                   // not a bug, this allows keys to be let go even if we are over the UI
    case InputEvent::MouseWheelForward:
        io.MouseWheel = 1;
        return io.WantCaptureMouse;
    case InputEvent::MouseWheelBackward:
        io.MouseWheel = -1;
        return io.WantCaptureMouse;
    default: break;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::ResetKeyDownState()
{
    ImGuiIO& io = ImGui::GetIO();
    for (uint32_t i = 0; i < ImGuiKey_COUNT; ++i)
    {
        if (KeysToRelease[i])
            io.KeysDown[i] = false;
        KeysToRelease[i] = false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
}

//------------------------------------------------------------------------------
/**
*/
void
ImguiContext::NewFrame(const Graphics::FrameContext& ctx)
{
    ImGuiIO& io = ImGui::GetIO();

    io.DeltaTime = ctx.frameTime;
    ImGui::GetStyle().Alpha = Core::CVarReadFloat(ui_opacity);
    ImGui::NewFrame();

}

} // namespace Dynui
