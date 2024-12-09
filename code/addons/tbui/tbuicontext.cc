//------------------------------------------------------------------------------
//  @file tbuicontext.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "math/mat4.h"
#include "input/inputserver.h"
#include "frame/default.h"
#include "io/assignregistry.h"
#include "input/keyboard.h"
#include "io/ioserver.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/addons/tbui/tbconfig.h"

#include <tb_config.h>
#include "tbuicontext.h"
#include "tbuiview.h"
#include "backend/tbuibatch.h"
#include "backend/tbuirenderer.h"
#include "backend/tbuifileinterface.h"
#include "backend/tbuisysteminterface.h"
#include "backend/tbuiclipboardinterface.h"
#include "backend/tbuifontrenderer.h"
#include "tb_core.h"
#include "tb_language.h"
#include "tb_font_renderer.h"
#include "animation/tb_widget_animation.h"
#include "tb_window.h"
#include "tb_node_tree.h"
#include "tb_widgets_reader.h"

#ifdef TB_FONT_RENDERER_TBBF
void register_tbbf_font_renderer();
#endif

namespace TBUI
{
namespace
{

//------------------------------------------------------------------------------
/**
*/
tb::MODIFIER_KEYS
GetModifierKeys()
{
    tb::MODIFIER_KEYS modifiers = tb::TB_MODIFIER_NONE;
    auto& kb = Input::InputServer::Instance()->GetDefaultKeyboard();
    if (kb->KeyDown(Input::Key::Shift))
    {
        modifiers |= tb::MODIFIER_KEYS::TB_SHIFT;
    }
    if (kb->KeyDown(Input::Key::Control))
    {
        modifiers |= tb::MODIFIER_KEYS::TB_CTRL;
    }
    if (kb->KeyDown(Input::Key::Menu))
    {
        modifiers |= tb::MODIFIER_KEYS::TB_ALT;
    }
    if (kb->KeyDown(Input::Key::LeftWindows) || kb->KeyDown(Input::Key::RightWindows))
    {
        modifiers |= tb::MODIFIER_KEYS::TB_SUPER;
    }

    return modifiers;
}

//------------------------------------------------------------------------------
/**
*/
tb::SPECIAL_KEY
GetSpecialKey(Input::Key::Code code)
{
    tb::SPECIAL_KEY key = tb::SPECIAL_KEY::TB_KEY_UNDEFINED;

    switch (code)
    {
    case Input::Key::Code::InvalidKey:
        return tb::TB_KEY_UNDEFINED;
    case Input::Key::Code::Up:
        return tb::TB_KEY_UP;
    case Input::Key::Code::Down:
        return tb::TB_KEY_DOWN;
    case Input::Key::Code::Left:
        return tb::TB_KEY_LEFT;
    case Input::Key::Code::Right:
        return tb::TB_KEY_RIGHT;
    case Input::Key::Code::PageUp:
        return tb::TB_KEY_PAGE_UP;
    case Input::Key::Code::PageDown:
        return tb::TB_KEY_PAGE_DOWN;
    case Input::Key::Code::Home:
        return tb::TB_KEY_HOME;
    case Input::Key::Code::End:
        return tb::TB_KEY_END;
    case Input::Key::Code::Tab:
        return tb::TB_KEY_TAB;
    case Input::Key::Code::Back:
        return tb::TB_KEY_BACKSPACE;
    case Input::Key::Code::Insert:
        return tb::TB_KEY_INSERT;
    case Input::Key::Code::Delete:
        return tb::TB_KEY_DELETE;
    case Input::Key::Code::Return:
        return tb::TB_KEY_ENTER;
    case Input::Key::Code::Escape:
        return tb::TB_KEY_ESC;
    case Input::Key::Code::F1:
        return tb::TB_KEY_F1;
    case Input::Key::Code::F2:
        return tb::TB_KEY_F2;
    case Input::Key::Code::F3:
        return tb::TB_KEY_F3;
    case Input::Key::Code::F4:
        return tb::TB_KEY_F4;
    case Input::Key::Code::F5:
        return tb::TB_KEY_F5;
    case Input::Key::Code::F6:
        return tb::TB_KEY_F6;
    case Input::Key::Code::F7:
        return tb::TB_KEY_F7;
    case Input::Key::Code::F8:
        return tb::TB_KEY_F8;
    case Input::Key::Code::F9:
        return tb::TB_KEY_F9;
    case Input::Key::Code::F10:
        return tb::TB_KEY_F10;
    case Input::Key::Code::F11:
        return tb::TB_KEY_F11;
    case Input::Key::Code::F12:
        return tb::TB_KEY_F12;
    }

    return key;
}

//------------------------------------------------------------------------------
/**
*/
static int32_t
GetTBKey(const Input::InputEvent& inputEvent)
{
    return (int32_t)Input::Key::ToChar(inputEvent.GetKey());
}
} // namespace

TBUIRenderer* TBUIContext::renderer = nullptr;
TBUIFileInterface TBUIContext::fileInterface;
TBUISystemInterface TBUIContext::systemInterface;
TBUIClipboardInterface TBUIContext::clipboardInterface;
TBUISTBFontRenderer* TBUIContext::stbFontRenderer;
Util::Array<TBUIView*> TBUIContext::views;
TBUIContext::TBUIState TBUIContext::state;
static constexpr size_t TBUI_VERTEX_MAX_SIZE = VERTEX_BATCH_SIZE * sizeof(TBUIVertex);

__ImplementPluginContext(TBUIContext)
    //------------------------------------------------------------------------------
    /**
*/
    TBUIContext::TBUIContext()
{
}

//------------------------------------------------------------------------------
/**
*/
TBUIContext::~TBUIContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIContext::Create()
{
    const IO::URI configPath = "config:tb/config.tbcf";
    if (!IO::IoServer::Instance()->FileExists(configPath))
    {
        return;
    }

    tbui::tbconfigT config;
    if (!Flat::FlatbufferInterface::DeserializeJsonFlatbuffer<tbui::tbconfig>(config, configPath, "tbconfig"))
    {
        return;
    }

    n_assert(FrameSync::FrameSyncTimer::HasInstance());
    __bundle.OnWindowResized = TBUIContext::OnWindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
    if (!tb::tb_core_is_initialized())
    {
        renderer = new TBUIRenderer();
        if (!tb::tb_core_init(renderer, &systemInterface, &fileInterface, &clipboardInterface))
        {
            delete renderer;
            return;
        }
        if (!IO::AssignRegistry::Instance()->HasAssign("tb"))
        {
            IO::AssignRegistry::Instance()->SetAssign(IO::Assign("tb", "data:turbobadger"));
        }

        // Load language file
        if (config.languages.size())
        {
            tb::g_tb_lng->Load(config.languages[0]->path.c_str());
        }
        // Register font renderers.
        register_tbbf_font_renderer();

        // Not great, but turbobadger will delete this
        // Maybe turbobadger should be patched to not delete this
        // Or it could allow our own destroy function to be passed in
        stbFontRenderer = new TBUISTBFontRenderer();
        tb::g_font_manager->AddRenderer(stbFontRenderer);
        
        for (const auto & font : config.fonts)
        {
            tb::g_font_manager->AddFontInfo(font->path.c_str(), font->name.c_str());
            if (config.default_font == font->name)
            {
                tb::TBFontDescription fd;
                fd.SetID(TBIDC(config.default_font.c_str()));
                if (font->scale > 0)
                {
                    fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(font->scale));
                }
                tb::g_font_manager->SetDefaultFontDescription(fd);
            }
        }
        tb::g_tb_skin->Load(config.default_skin.c_str(), config.skin.c_str());

        // Create the font now.
        tb::TBFontFace* font = tb::g_font_manager->CreateFontFace(tb::g_font_manager->GetDefaultFontDescription());

        // Render some glyphs in one go now since we know we are going to use them. It would work fine
        // without this since glyphs are rendered when needed, but with some extra updating of the glyph bitmap.
        if (font)
            font->RenderGlyphs(
                " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ï∑Â‰ˆ≈ƒ÷"
            );

        tb::TBWidgetsAnimationManager::Init();

        // setup state
        {
            // allocate shader
            state.shader = CoreGraphics::ShaderGet("shd:tbui.fxb");
            state.shaderProgram = CoreGraphics::ShaderGetProgram(state.shader, CoreGraphics::ShaderFeatureMask("Static"));

            state.resourceTable = CoreGraphics::ShaderCreateResourceTable(state.shader, NEBULA_BATCH_GROUP);

            state.textProjectionConstant = CoreGraphics::ShaderGetConstantBinding(state.shader, "TextProjectionModel");
            state.textureConstant = CoreGraphics::ShaderGetConstantBinding(state.shader, "Texture");

            // create vertex buffer
            Util::Array<CoreGraphics::VertexComponent> components;
            components.Append(CoreGraphics::VertexComponent(0, CoreGraphics::VertexComponent::Float2, 0));
            components.Append(CoreGraphics::VertexComponent(1, CoreGraphics::VertexComponent::Float2, 0));
            components.Append(CoreGraphics::VertexComponent(2, CoreGraphics::VertexComponent::UByte4N, 0));
            state.vertexLayout = CoreGraphics::CreateVertexLayout({.name = "TBUI Vertex Layout", .comps = components});

            FrameScript_default::RegisterSubgraphPipelines_StaticUIToBackbuffer_Pass(
                [](const CoreGraphics::PassId pass, uint subpass)
                {
                    CoreGraphics::InputAssemblyKey inputAssembly {CoreGraphics::PrimitiveTopology::TriangleList, false};
                    if (state.pipeline != CoreGraphics::InvalidPipelineId)
                        CoreGraphics::DestroyGraphicsPipeline(state.pipeline);
                    state.pipeline = CoreGraphics::CreateGraphicsPipeline({state.shaderProgram, pass, subpass, inputAssembly});
                }
            );

            FrameScript_default::RegisterSubgraph_StaticUIToBackbuffer_Pass(
                [](const CoreGraphics::CmdBufferId cmdBuf,
                   const Math::rectangle<int>& viewport,
                   const IndexT frame,
                   const IndexT bufferIndex)
                {
                    //TBUI::TBUIContext::FrameUpdate({});
                    TBUIContext::Render(cmdBuf, viewport, frame, bufferIndex);
                }
            );

            SizeT numBuffers = CoreGraphics::GetNumBufferedFrames();

            CoreGraphics::BufferCreateInfo vboInfo;
            vboInfo.name = "TBUI VBO"_atm;
            vboInfo.size = 1;
            vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(state.vertexLayout);
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

            // map buffer
            state.vertexPtrs.Resize(numBuffers);
            for (i = 0; i < numBuffers; i++)
            {
                state.vertexPtrs[i] = (byte*)CoreGraphics::BufferMap(state.vbos[i]);
            }

            state.inputHandler = TBUIInputHandler::Create();
            Input::InputServer::Instance()->AttachInputHandler(
                Input::InputPriority::Gui, state.inputHandler.upcast<Input::InputHandler>()
            );

            state.timer = FrameSync::FrameSyncTimer::Instance();
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIContext::Discard()
{
    if (tb::tb_core_is_initialized())
    {
        tb::TBWidgetsAnimationManager::Shutdown();
        tb::tb_core_shutdown();

        Input::InputServer::Instance()->RemoveInputHandler(state.inputHandler.upcast<Input::InputHandler>());
        state.inputHandler = nullptr;

        IndexT i;
        for (i = 0; i < state.vbos.Size(); i++)
        {
            CoreGraphics::BufferUnmap(state.vbos[i]);

            CoreGraphics::DestroyBuffer(state.vbos[i]);

            state.vertexPtrs[i] = nullptr;
        }

        delete renderer;

        IO::AssignRegistry::Instance()->ClearAssign("tb");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIContext::FrameUpdate(const Graphics::FrameContext& ctx)
{
    tb::TBMessageHandler::ProcessMessages();
    tb::TBAnimationManager::Update();

    for (auto& view : views)
    {
        view->InvokeProcessStates();
        view->InvokeProcess();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIContext::OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    for (auto& view : views)
    {
        //view->SetSize(width, height);
    }
}

//------------------------------------------------------------------------------
/**
*/
TBUIView*
TBUIContext::CreateView(int32_t width, int32_t height)
{
    TBUIView* view = new TBUIView();
    {
        view->Invalidate();
        view->SetSize(width, height);
        view->InvalidateLayout(tb::TBWidget::INVALIDATE_LAYOUT_RECURSIVE);

        // Set gravity all so we resize correctly
        view->SetGravity(tb::WIDGET_GRAVITY_ALL);

        view->SetFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);
    }
    views.Append(view);
    return view;
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIContext::DestroyView(const TBUIView* view)
{
    if (auto it = views.Find(const_cast<TBUIView*>(view)))
    {
        views.Erase(it);
        (*it)->Die();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
TBUIContext::ProcessInput(const Input::InputEvent& inputEvent)
{
    if (views.IsEmpty())
        return false;

    TBUIView* view = views.Back();

    if (!view)
        return false;

    tb::MODIFIER_KEYS modifiers = GetModifierKeys();

    switch (inputEvent.GetType())
    {
    case Input::InputEvent::KeyUp: {
        return view->InvokeKey(/*GetTBKey(inputEvent)*/ 0, GetSpecialKey(inputEvent.GetKey()), modifiers, false);
    }
    break;
    case Input::InputEvent::KeyDown: {
        return view->InvokeKey(/*GetTBKey(inputEvent)*/ 0, GetSpecialKey(inputEvent.GetKey()), modifiers, true);
    }
    case Input::InputEvent::Character: {
        int c = (int)inputEvent.GetChar();
        tb::SPECIAL_KEY key = GetSpecialKey(inputEvent.GetKey());
        view->InvokeKey(c, key, modifiers, false);
        return view->InvokeKey(c, key, modifiers, true);
    }
    break;

    case Input::InputEvent::MouseButtonDown: {
        inputEvent.GetMouseButton();
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        return view->InvokePointerDown(pos.x, pos.y, 1, modifiers, false);
    }
    case Input::InputEvent::MouseButtonUp: {
        inputEvent.GetMouseButton();
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        return view->InvokePointerUp(pos.x, pos.y, modifiers, false);
    }
    case Input::InputEvent::MouseMove: {
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        view->InvokePointerMove(pos.x, pos.y, modifiers, false);
    }
    break;
    case Input::InputEvent::MouseWheelForward: {
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        view->InvokeWheel(pos.x, pos.y, 0, 1, modifiers);
    }
    break;
    case Input::InputEvent::MouseWheelBackward: {
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        view->InvokeWheel(pos.x, pos.y, 0, -1, modifiers);
    }
    break;
    default:
        break;
        {
        }
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
TBUIContext::Render(
    const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex
)
{
    if (views.IsEmpty())
        return;

    Util::Array<TBUIBatch> batches;

    // todo: Maybe render only the top view?
    renderer->SetCmdBufferId(cmdBuf);

    for (const auto& view : views)
    {
        batches = renderer->RenderView(view, viewport.width(), viewport.height());
    }

    IndexT currentBuffer = CoreGraphics::GetBufferedFrameIndex();
    CoreGraphics::BufferId vbo = TBUIContext::state.vbos[currentBuffer];

    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_GRAPHICS, "TBUI");

    // create orthogonal matrix
#if __VULKAN__
    Math::mat4 proj = Math::orthooffcenterrh(0.0f, viewport.width(), viewport.height(), 0.0f, -1.0f, +1.0f);
#else
    Math::mat4 proj = Math::orthooffcenterrh(0.0f, viewport.width(), 0.0f, viewport.height(), -1.0f, +1.0f);
#endif

    Math::mat4 transform = Math::mat4::identity;

    proj = proj * transform;

    size_t requiredVertexBufferSize = 0;

    for (const auto& batch : batches)
    {
        requiredVertexBufferSize += sizeof(TBUIVertex) * batch.vertices.Size();
    }

    // if buffers are too small, create new buffers
    if (requiredVertexBufferSize > CoreGraphics::BufferGetByteSize(TBUIContext::state.vbos[currentBuffer]))
    {
        CoreGraphics::BufferUnmap(TBUIContext::state.vbos[currentBuffer]);
        CoreGraphics::DestroyBuffer(TBUIContext::state.vbos[currentBuffer]);

        CoreGraphics::BufferCreateInfo vboInfo;
        vboInfo.name = "TBUI VBO"_atm;
        vboInfo.size = 0;
        vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(TBUIContext::state.vertexLayout);
        vboInfo.byteSize = requiredVertexBufferSize;
        vboInfo.mode = CoreGraphics::HostCached;
        vboInfo.usageFlags = CoreGraphics::VertexBuffer;
        vboInfo.data = nullptr;
        vboInfo.dataSize = 0;
        TBUIContext::state.vbos[currentBuffer] = CoreGraphics::CreateBuffer(vboInfo);
        TBUIContext::state.vertexPtrs[currentBuffer] = (byte*)CoreGraphics::BufferMap(TBUIContext::state.vbos[currentBuffer]);
    }

    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, state.pipeline);
    CoreGraphics::CmdSetVertexLayout(cmdBuf, state.vertexLayout);
    CoreGraphics::CmdSetResourceTable(cmdBuf, state.resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
    CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, TBUIContext::state.vbos[currentBuffer], 0);
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::CmdSetViewport(cmdBuf, viewport, 0);
    CoreGraphics::CmdSetScissorRect(cmdBuf, viewport, 0);

    // set projection
    CoreGraphics::CmdPushConstants(
        cmdBuf, CoreGraphics::GraphicsPipeline, state.textProjectionConstant, sizeof(proj), (byte*)&proj
    );

    IndexT vertexOffset = 0;
    IndexT vertexBufferOffset = 0;

    for (IndexT i = 0; i < batches.Size(); i++)
    {
        const auto& batch = batches[i];
        const unsigned char* vertexBuffer = (unsigned char*)&batch.vertices.Front();
        const SizeT vertexBufferSize = batch.vertices.Size() * sizeof(TBUIVertex); // 2 for position, 2 for uvs, 1 int for color

        // if we render too many vertices, we will simply assert, but should never happen really
        n_assert(
            vertexBufferOffset + (IndexT)batch.vertices.Size() <
            CoreGraphics::BufferGetByteSize(TBUIContext::state.vbos[currentBuffer])
        );

        // wait for previous draws to finish...
        Memory::Copy(vertexBuffer, TBUIContext::state.vertexPtrs[currentBuffer] + vertexBufferOffset, vertexBufferSize);

        CoreGraphics::CmdSetScissorRect(cmdBuf, batch.clipRect, 0);

        IndexT textureId = CoreGraphics::TextureGetBindlessHandle(batch.texture);

        CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, state.textureConstant, sizeof(IndexT), &textureId);

        // setup primitive
        CoreGraphics::PrimitiveGroup primitive;
        primitive.SetNumIndices(0);
        primitive.SetBaseIndex(0);
        primitive.SetBaseVertex(vertexOffset);
        primitive.SetNumVertices(batch.vertices.Size());

        // prepare render device and draw
        CoreGraphics::CmdDraw(cmdBuf, primitive);

        // bump vertices
        vertexOffset += batch.vertices.Size();

        // lock buffers
        vertexBufferOffset += vertexBufferSize;
    }
    CoreGraphics::BufferFlush(TBUIContext::state.vbos[currentBuffer]);
}

} // namespace TBUI
