//------------------------------------------------------------------------------
//  @file tbuicontext.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/mat4.h"
#include "input/inputserver.h"
#include "tbuicontext.h"
#include "tbuiview.h"
#include "backend/tbuibatch.h"
#include "backend/tbuirenderer.h"
#include "tb/tb_core.h"
#include "tb/tb_language.h"
#include "tb/tb_font_renderer.h"
#include "tb/animation/tb_widget_animation.h"
#include "tb/tb_window.h"
#include "tb/tb_node_tree.h"
#include "tb/tb_widgets_reader.h"
#include "frame/default.h"
#include "io/assignregistry.h"

namespace TBUI
{
namespace
{
tb::MODIFIER_KEYS
GetModifierKeys(const Input::InputEvent& inputEvent)
{
    tb::MODIFIER_KEYS modifiers = tb::TB_MODIFIER_NONE;

    return modifiers;
}

tb::SPECIAL_KEY
GetSpecialKey()
{
}
} // namespace

TBUIRenderer* TBUIContext::renderer = nullptr;
Ptr<TBUIInputHandler> TBUIContext::inputHandler;
Util::Array<TBUIView*> TBUIContext::views;
CoreGraphics::VertexLayoutId TBUIContext::vertexLayout;
CoreGraphics::ShaderId TBUIContext::shader;
CoreGraphics::ShaderProgramId TBUIContext::shaderProgram;
CoreGraphics::ResourceTableId TBUIContext::resourceTable;
CoreGraphics::PipelineId TBUIContext::pipeline;
IndexT TBUIContext::textProjectionConstant;
IndexT TBUIContext::textureConstant;

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
    __bundle.OnWindowResized = TBUIContext::OnWindowResized;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
    if (!tb::tb_core_is_initialized())
    {
        renderer = new TBUIRenderer();
        if (!tb::tb_core_init(renderer))
        {
            delete renderer;
            return;
        }

        IO::AssignRegistry::Instance()->SetAssign(IO::Assign("tb", "root:work/turbobadger"));

        // allocate shader
        shader = CoreGraphics::ShaderGet("shd:tbui.fxb");
        shaderProgram = CoreGraphics::ShaderGetProgram(shader, CoreGraphics::ShaderFeatureMask("Static"));
        resourceTable = CoreGraphics::ShaderCreateResourceTable(shader, NEBULA_BATCH_GROUP);

        textProjectionConstant = CoreGraphics::ShaderGetConstantBinding(shader, "TextProjectionModel");
        textureConstant = CoreGraphics::ShaderGetConstantBinding(shader, "Texture");

        // create vertex buffer
        Util::Array<CoreGraphics::VertexComponent> components;
        components.Append(CoreGraphics::VertexComponent(0, CoreGraphics::VertexComponent::Float2, 0));
        components.Append(CoreGraphics::VertexComponent(1, CoreGraphics::VertexComponent::Float2, 0));
        components.Append(CoreGraphics::VertexComponent(2, CoreGraphics::VertexComponent::UByte4N, 0));
        vertexLayout = CoreGraphics::CreateVertexLayout({.name = "TBUI Vertex Layout", .comps = components});

        inputHandler = TBUIInputHandler::Create();
        Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::Gui, inputHandler.upcast<Input::InputHandler>());

        // Load language file
        tb::g_tb_lng->Load("tb:resources/language/lng_en.tb.txt");

        // Load the default skin
        tb::g_tb_skin->Load("tb:resources/default_skin/skin.tb.txt", "tb:demo/skin/skin.tb.txt");

        // Register font renderers.
#ifdef TB_FONT_RENDERER_TBBF
        void register_tbbf_font_renderer();
        register_tbbf_font_renderer();
#endif

#ifdef TB_FONT_RENDERER_STB
        void register_stb_font_renderer();
        register_stb_font_renderer();
#endif
#ifdef TB_FONT_RENDERER_FREETYPE
        void register_freetype_font_renderer();
        register_freetype_font_renderer();
#endif

        // Add fonts we can use to the font manager.
#if defined(TB_FONT_RENDERER_STB) || defined(TB_FONT_RENDERER_FREETYPE)
        tb::g_font_manager->AddFontInfo("tb:resources/vera.ttf", "Vera");
#endif
#ifdef TB_FONT_RENDERER_TBBF
        tb::g_font_manager->AddFontInfo("tb:resources/default_font/segoe_white_with_shadow.tb.txt", "Segoe");
        tb::g_font_manager->AddFontInfo("tb:demo/fonts/neon.tb.txt", "Neon");
        tb::g_font_manager->AddFontInfo("tb:demo/fonts/orangutang.tb.txt", "Orangutang");
        tb::g_font_manager->AddFontInfo("tb:demo/fonts/orange.tb.txt", "Orange");
#endif

        // Set the default font description for widgets to one of the fonts we just added
        tb::TBFontDescription fd;
#ifdef TB_FONT_RENDERER_TBBF
        fd.SetID(TBIDC("Segoe"));
#else
        fd.SetID(TBIDC("Vera"));
#endif
        fd.SetSize(tb::g_tb_skin->GetDimensionConverter()->DpToPx(14));
        tb::g_font_manager->SetDefaultFontDescription(fd);

        // Create the font now.
        tb::TBFontFace* font = tb::g_font_manager->CreateFontFace(tb::g_font_manager->GetDefaultFontDescription());

        // Render some glyphs in one go now since we know we are going to use them. It would work fine
        // without this since glyphs are rendered when needed, but with some extra updating of the glyph bitmap.
        if (font)
            font->RenderGlyphs(" !\"#$%&'()*+,-./"
                               "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~ï∑Â‰ˆ≈ƒ÷");

        FrameScript_default::RegisterSubgraphPipelines_StaticUIToBackbuffer_Pass(
            [](const CoreGraphics::PassId pass, uint subpass)
            {
                CoreGraphics::InputAssemblyKey inputAssembly {CoreGraphics::PrimitiveTopology::TriangleList, false};
                if (pipeline != CoreGraphics::InvalidPipelineId)
                    CoreGraphics::DestroyGraphicsPipeline(pipeline);
                pipeline = CoreGraphics::CreateGraphicsPipeline({shaderProgram, pass, subpass, inputAssembly});
            }
        );

        FrameScript_default::RegisterSubgraph_StaticUIToBackbuffer_Pass(
            [](const CoreGraphics::CmdBufferId cmdBuf,
               const Math::rectangle<int>& viewport,
               const IndexT frame,
               const IndexT bufferIndex)
            {
                TBUIContext::Render(cmdBuf, viewport, frame, bufferIndex);
            }
        );

        tb::TBWidgetsAnimationManager::Init();
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

        IO::AssignRegistry::Instance()->ClearAssign("tb");

        Input::InputServer::Instance()->RemoveInputHandler(inputHandler.upcast<Input::InputHandler>());
        inputHandler = nullptr;

        delete renderer;
    }
}

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

void
TBUIContext::OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    for (auto& view : views)
    {
        //view->SetSize(width, height);
    }
}

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

void
TBUIContext::DestroyView(const TBUIView* view)
{
    if (auto it = views.Find(const_cast<TBUIView*>(view)))
    {
        views.Erase(it);
        (*it)->Die();
    }
}

bool
TBUIContext::ProcessInput(const Input::InputEvent& inputEvent)
{
    TBUIView* view = views.Back();
    if (!view)
        return false;

    switch (inputEvent.GetType())
    {
    case Input::InputEvent::MouseButtonDown: {
        inputEvent.GetMouseButton();
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        return view->InvokePointerDown(pos.x, pos.y, 1, GetModifierKeys(inputEvent), false);
    }
    case Input::InputEvent::MouseButtonUp: {
        inputEvent.GetMouseButton();
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        return view->InvokePointerUp(pos.x, pos.y, GetModifierKeys(inputEvent), false);
    }
    case Input::InputEvent::MouseMove: {
        Math::vec2 pos = inputEvent.GetAbsMousePos();
        view->InvokePointerMove(pos.x, pos.y, GetModifierKeys(inputEvent), false);
    }
    default:
        break;
        {
        }
    }

    return false;
}

Util::Array<CoreGraphics::BufferId> TBUIContext::usedVertexBuffers;

void
TBUIContext::Render(
    const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex
)
{
    // create orthogonal matrix
#if __VULKAN__
    Math::mat4 proj = Math::orthooffcenterrh(0.0f, viewport.width(), viewport.height(), 0.0f, -1.0f, +1.0f);
#else
    Math::mat4 proj = Math::orthooffcenterrh(0.0f, viewport.width(), 0.0f, viewport.height(), -1.0f, +1.0f);
#endif

    Math::mat4 transform = Math::mat4::identity;

    proj = proj * transform;

    {
        // Destroy previously used vertex buffers
        // todo: use pool instead of creating and destroying each render call

        for (const auto& vertexBuffer : usedVertexBuffers)
        {
            CoreGraphics::DestroyBuffer(vertexBuffer);
        }
        usedVertexBuffers.Clear();
    }

    Util::Array<TBUIBatch> batches;

    // todo: Maybe render only the top view?
    renderer->SetCmdBufferId(cmdBuf);
    for (const auto& view : views)
    {
        batches = renderer->RenderView(view, viewport.width(), viewport.height());
    }

    //CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, pipeline);
    CoreGraphics::CmdSetVertexLayout(cmdBuf, vertexLayout);
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::CmdSetViewport(cmdBuf, viewport, 0);
    CoreGraphics::CmdSetScissorRect(cmdBuf, viewport, 0);

    // set projection
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, textProjectionConstant, sizeof(proj), (byte*)&proj);

    for (auto& batch : batches)
    {
        // todo: user buffer pool
        CoreGraphics::BufferCreateInfo vboInfo;
        vboInfo.name = "TBUI VBO"_atm;
        vboInfo.size = batch.vertices.Size();
        vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(vertexLayout);
        vboInfo.mode = CoreGraphics::HostCached;
        vboInfo.usageFlags = CoreGraphics::VertexBuffer;
        vboInfo.data = &batch.vertices.Front();
        vboInfo.dataSize = sizeof(TBUIVertex) * batch.vertices.Size();
        CoreGraphics::BufferId vertexBuffer = CoreGraphics::CreateBuffer(vboInfo);

        CoreGraphics::CmdSetResourceTable(cmdBuf, resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
        CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, vertexBuffer, 0);
        CoreGraphics::CmdSetScissorRect(cmdBuf, batch.clipRect, 0);

        IndexT textureId = CoreGraphics::TextureGetBindlessHandle(batch.texture);

        CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, textureConstant, sizeof(IndexT), &textureId);

        // setup primitive
        CoreGraphics::PrimitiveGroup primitive;
        primitive.SetNumIndices(0);
        primitive.SetBaseIndex(0);
        primitive.SetBaseVertex(0);
        primitive.SetNumVertices(batch.vertices.Size());

        // prepare render device and draw
        CoreGraphics::CmdDraw(cmdBuf, primitive);

        CoreGraphics::BufferFlush(vertexBuffer);

        usedVertexBuffers.Append(vertexBuffer);
    }
}

} // namespace TBUI
