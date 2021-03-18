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
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "frame/frameplugin.h"
#include "core/cvar.h"

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
ImguiContext::ImguiDrawFunction()
{
    ImDrawData* data = ImGui::GetDrawData();
    // get Imgui context
    ImGuiIO& io = ImGui::GetIO();
    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    data->ScaleClipRects(io.DisplayFramebufferScale);

    // get renderer 
    //const Ptr<BufferLock>& vboLock = renderer->GetVertexBufferLock();
    //const Ptr<BufferLock>& iboLock = renderer->GetIndexBufferLock();
    IndexT currentBuffer = CoreGraphics::GetBufferedFrameIndex();
    BufferId vbo = state.vbos[currentBuffer];
    BufferId ibo = state.ibos[currentBuffer];
    const ImguiRendererParams& params = state.params;

    // apply shader
    CoreGraphics::SetShaderProgram(state.prog);

    // create orthogonal matrix
#if __VULKAN__
    mat4 proj = orthooffcenterrh(0.0f, io.DisplaySize.x, 0.0f, io.DisplaySize.y, -1.0f, +1.0f);
#else
    mat4 proj = orthooffcenterrh(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
#endif

	// if buffers are too small, create new buffers
    if (data->TotalVtxCount > CoreGraphics::BufferGetSize(state.vbos[currentBuffer]))
    {
		CoreGraphics::BufferUnmap(state.vbos[currentBuffer]);
		CoreGraphics::DestroyBuffer(state.vbos[currentBuffer]);

		CoreGraphics::BufferCreateInfo vboInfo;
		vboInfo.name = "ImGUI VBO"_atm;
		vboInfo.size = Math::roundtopow2(data->TotalVtxCount);
		vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(state.vlo);
		vboInfo.mode = CoreGraphics::HostToDevice;
		vboInfo.usageFlags = CoreGraphics::VertexBuffer;
		vboInfo.data = nullptr;
		vboInfo.dataSize = 0;
		state.vbos[currentBuffer] = CoreGraphics::CreateBuffer(vboInfo);
		state.vertexPtrs[currentBuffer] = (byte*)CoreGraphics::BufferMap(state.vbos[currentBuffer]);
    }

	if (data->TotalIdxCount > CoreGraphics::BufferGetSize(state.ibos[currentBuffer]))
    {
		CoreGraphics::BufferUnmap(state.ibos[currentBuffer]);
		CoreGraphics::DestroyBuffer(state.ibos[currentBuffer]);

		CoreGraphics::BufferCreateInfo iboInfo;
		iboInfo.name = "ImGUI IBO"_atm;
		iboInfo.size = Math::roundtopow2(data->TotalIdxCount);
		iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(IndexType::Index16);
		iboInfo.mode = CoreGraphics::HostToDevice;
		iboInfo.usageFlags = CoreGraphics::IndexBuffer;
		iboInfo.data = nullptr;
		iboInfo.dataSize = 0;
		state.ibos[currentBuffer] = CoreGraphics::CreateBuffer(iboInfo);
		state.indexPtrs[currentBuffer] = (byte*)CoreGraphics::BufferMap(state.ibos[currentBuffer]);
    }

    // setup device
    CoreGraphics::SetVertexLayout(state.vlo);
    CoreGraphics::SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::SetGraphicsPipeline();

    // setup input buffers
    CoreGraphics::SetResourceTable(state.resourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);
    CoreGraphics::SetStreamVertexBuffer(0, state.vbos[currentBuffer], 0);
    CoreGraphics::SetIndexBuffer(state.ibos[currentBuffer], 0);

    // set projection
    CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, state.textProjectionConstant, sizeof(proj), (byte*)&proj);

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
        n_assert(vertexBufferOffset + (IndexT)commandList->VtxBuffer.size() < CoreGraphics::BufferGetByteSize(state.vbos[currentBuffer]));
        n_assert(indexBufferOffset + (IndexT)commandList->IdxBuffer.size() < CoreGraphics::BufferGetByteSize(state.ibos[currentBuffer]));

        // wait for previous draws to finish...
		Memory::Copy(vertexBuffer, state.vertexPtrs[currentBuffer] + vertexBufferOffset, vertexBufferSize);
		Memory::Copy(indexBuffer, state.indexPtrs[currentBuffer] + indexBufferOffset, indexBufferSize);
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
                CoreGraphics::SetScissorRect(scissorRect, 0);
                ImguiTextureId tex = *(ImguiTextureId*)command->TextureId;

                TextureInfo texInfo;
                texInfo.type = 0;
                texInfo.useAlpha = 1;

                // set texture in shader, we shouldn't have to put it into ImGui
                CoreGraphics::TextureId texture = (CoreGraphics::TextureId)tex.nebulaHandle;
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
                
                CoreGraphics::PushConstants(CoreGraphics::GraphicsPipeline, state.packedTextureInfo, sizeof(TextureInfo), (byte*)& texInfo);

                // setup primitive
                CoreGraphics::PrimitiveGroup primitive;
                primitive.SetNumIndices(command->ElemCount);
                primitive.SetBaseIndex(primitiveIndexOffset + indexOffset);
                primitive.SetBaseVertex(vertexOffset);

                CoreGraphics::SetPrimitiveGroup(primitive);

                // prepare render device and draw
                CoreGraphics::Draw();
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

    CoreGraphics::BufferFlush(state.vbos[currentBuffer]);
    CoreGraphics::BufferFlush(state.ibos[currentBuffer]);

    // reset clip settings
    CoreGraphics::ResetClipSettings();
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
            if (verbose) LogWarning("Recovered from missing EndTable() call.");
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
    ui_opacity = Core::CVarCreate(Core::CVar_Float, "ui_opacity", "1.0");

    __bundle.OnBegin = ImguiContext::OnBeforeFrame;
    __bundle.OnWindowResized = ImguiContext::OnWindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    state.dockOverViewport = false;

    // allocate imgui shader
    state.uiShader = ShaderServer::Instance()->GetShader("shd:imgui.fxb");
    state.params.projVar = CoreGraphics::ShaderGetConstantBinding(state.uiShader,"TextProjectionModel");
    state.params.fontVar = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "Texture");
    state.prog = CoreGraphics::ShaderGetProgram(state.uiShader, CoreGraphics::ShaderFeatureFromString("Static"));

    state.resourceTable = CoreGraphics::ShaderCreateResourceTable(state.uiShader, NEBULA_BATCH_GROUP);

    state.textureConstant = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "Texture");
    state.textProjectionConstant = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "TextProjectionModel");
    state.packedTextureInfo = CoreGraphics::ShaderGetConstantBinding(state.uiShader, "PackedTextureInfo");

    state.inputHandler = ImguiInputHandler::Create();
    Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, state.inputHandler.upcast<Input::InputHandler>());

    // create vertex buffer
    Util::Array<CoreGraphics::VertexComponent> components;
    components.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponentBase::Float2, 0));
    components.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponentBase::Float2, 0));
    components.Append(VertexComponent((VertexComponent::SemanticName)2, 0, VertexComponentBase::UByte4N, 0));
    state.vlo = CoreGraphics::CreateVertexLayout({ components });

    Frame::AddCallback("ImGUI", [](const IndexT frame, const IndexT bufferIndex)
        {
            //ImGui::End();
#ifdef NEBULA_NO_DYNUI_ASSERTS
            ImguiContext::RecoverImGuiContextErrors();
#endif

            CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
            ImGui::Render();
            ImguiContext::ImguiDrawFunction();
            CoreGraphics::EndBatch();
        });

    SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "ImGUI VBO"_atm;
    vboInfo.size = 1;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(state.vlo);
    vboInfo.mode = CoreGraphics::HostToDevice;
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
    iboInfo.mode = CoreGraphics::HostToDevice;
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
    Ptr<DisplayDevice> display = DisplayDevice::Instance();
    DisplayMode mode = CoreGraphics::WindowGetDisplayMode(display->GetCurrentWindow());

    // setup Imgui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)mode.GetWidth(), (float)mode.GetHeight());
    io.DeltaTime = 1 / 60.0f;
    //io.PixelCenterOffset = 0.0f;
    //io.FontTexUvForWhite = ImVec2(1, 1);
    //io.RenderDrawListsFn = ImguiDrawFunction;

    ImGuiStyle& style = ImGui::GetStyle();
    
    style.FrameRounding = 2.0f;
    style.GrabRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.WindowRounding = 2.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding = 3.0f;

    style.WindowTitleAlign = { 0.5f, 0.38f };
    style.WindowMenuButtonPosition = ImGuiDir_Right;

    style.WindowPadding = { 8.0f, 8.0f };
    style.FramePadding = { 16, 3 };
    style.ItemInnerSpacing = { 4, 2 };
    style.ItemSpacing = { 4, 5 };
    style.IndentSpacing = 8.0f;
    style.GrabMinSize = 8.0f;

    style.FrameBorderSize = 0.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;

    ImVec4 nebulaOrange(1.0f, 0.30f, 0.0f, 1.0f);
    ImVec4 nebulaOrangeActive(0.9f, 0.20f, 0.05f, 1.0f);
    ImVec4* colors = ImGui::GetStyle().Colors;
    ImGui::GetStyle().Alpha = Core::CVarReadFloat(ui_opacity);
    colors[ImGuiCol_Text]                   = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.27f, 0.27f, 0.27f, 0.50f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.09f, 0.09f, 0.09f, 0.59f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.95f);
    colors[ImGuiCol_Border]                 = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.18f, 0.25f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.11f, 0.11f, 0.11f, 0.89f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(1.00f, 0.30f, 0.00f, 0.07f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(1.00f, 0.40f, 0.00f, 0.38f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(1.00f, 0.30f, 0.00f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(1.00f, 0.30f, 0.00f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 0.47f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.30f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(1.00f, 0.30f, 0.00f, 0.70f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(1.00f, 0.30f, 0.00f, 0.90f);
    colors[ImGuiCol_Header]                 = ImVec4(1.00f, 0.30f, 0.00f, 0.60f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(1.00f, 0.30f, 0.00f, 0.70f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(1.00f, 0.30f, 0.00f, 0.90f);
    colors[ImGuiCol_Separator]              = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(1.00f, 0.30f, 0.00f, 0.70f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.90f, 0.20f, 0.05f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 0.30f, 0.00f, 0.11f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.00f, 0.30f, 0.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 0.30f, 0.00f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.04f, 0.04f, 0.04f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.03f, 0.03f, 0.03f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.16f, 0.16f, 0.16f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(1.00f, 0.30f, 0.00f, 0.23f);
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

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
    ImFont* font = io.Fonts->AddFontFromFileTTF("c:/windows/fonts/segoeui.ttf", 17, &config);
#else
    ImFont* font = io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 18, &config);
#endif
    
    unsigned char* buffer;
    int width, height, channels;
    io.Fonts->GetTexDataAsRGBA32(&buffer, &width, &height, &channels);

    // load image using SOIL
    // unsigned char* texData = SOIL_load_image_from_memory(buffer, width * height * channels, &width, &height, &channels, SOIL_LOAD_AUTO);

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "imgui_font_tex"_atm;
    texInfo.usage = TextureUsage::SampleTexture;
    texInfo.tag = "system"_atm;
    texInfo.buffer = buffer;
    texInfo.type = TextureType::Texture2D;
    texInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    texInfo.width = width;
    texInfo.height = height;

    state.fontTexture.nebulaHandle = CoreGraphics::CreateTexture(texInfo).HashCode64();
    state.fontTexture.mip = 0;
    state.fontTexture.layer = 0;
    io.Fonts->TexID = &state.fontTexture;
    io.Fonts->ClearTexData();

    // load settings from disk. If we don't do this here we need to
    // run an entire frame before being able to create or load settings
    if (!IO::IoServer::Instance()->FileExists("imgui.ini"))
    {
        ImGui::SaveIniSettingsToDisk("imgui.ini");
    }
    ImGui::LoadIniSettingsFromDisk("imgui.ini");
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
        io.KeysDown[event.GetKey()] = false;
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
    }
    
    return false;
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
ImguiContext::OnBeforeFrame(const Graphics::FrameContext& ctx)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = ctx.frameTime;
    ImGui::NewFrame();
    ImGui::GetStyle().Alpha = Core::CVarReadFloat(ui_opacity);
#ifdef IMGUI_HAS_DOCK
    if (state.dockOverViewport)
        ImGui::DockSpaceOverViewport();
#endif

}

} // namespace Dynui
