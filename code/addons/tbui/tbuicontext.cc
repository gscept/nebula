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
#include "frame/default.h"
#include "io/assignregistry.h"

namespace TBUI
{
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

        Input::InputServer::Instance()->RemoveInputHandler(inputHandler.upcast<Input::InputHandler>());
        inputHandler = nullptr;

        delete renderer;
    }
}

void
TBUIContext::OnWindowResized(const CoreGraphics::WindowId windowId, SizeT width, SizeT height)
{
    for (auto& view : views)
    {
        view->SetSize(width, height);
    }
}

TBUIView*
TBUIContext::CreateView()
{
    TBUIView* view = new TBUIView();
    views.Append(view);
    return view;
}

void
TBUIContext::DestroyView(const TBUIView* view)
{
    if (auto it = views.Find(const_cast<TBUIView*>(view)))
        views.Erase(it);

    delete view;
}

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

    // todo: Maybe render only the top view?
    renderer->SetCmdBufferId(cmdBuf);
    renderer->BeginBatch();
    for (auto& view : views)
    {
        tb::TBRect clipRect = view->GetRect();
        //renderer->SetClipRect(clipRect);
        renderer->BeginPaint(clipRect.w, clipRect.h);
        view->InvokePaint(tb::TBWidget::PaintProps());
        renderer->EndPaint();
    }
    const Util::Array<TBUIBatch>& batches = renderer->EndBatch();

    //CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, pipeline);
    CoreGraphics::CmdSetVertexLayout(cmdBuf, vertexLayout);
    CoreGraphics::CmdSetResourceTable(cmdBuf, resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::TriangleList);
    CoreGraphics::CmdSetViewport(cmdBuf, viewport, 0);
    CoreGraphics::CmdSetScissorRect(cmdBuf, viewport, 0);

    // set projection
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, textProjectionConstant, sizeof(proj), (byte*)&proj);

    for (auto& batch : batches)
    {
        // todo: user buffer pool
        // todo: prepare buffer during batch creation instead of creating here
        CoreGraphics::BufferCreateInfo vboInfo;
        vboInfo.name = "TBUI VBO"_atm;
        vboInfo.size = batch.vertices.Size();
        vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(vertexLayout);
        vboInfo.mode = CoreGraphics::HostCached;
        vboInfo.usageFlags = CoreGraphics::VertexBuffer;
        vboInfo.data = &batch.vertices.Front();
        vboInfo.dataSize = sizeof(TBUIVertex) * batch.vertices.Size();
        CoreGraphics::BufferId vertexBuffer = CoreGraphics::CreateBuffer(vboInfo);

        CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, vertexBuffer, 0);
        CoreGraphics::CmdSetScissorRect(cmdBuf, batch.clipRect, 0);

        IndexT textureId = CoreGraphics::TextureGetBindlessHandle(batch.texture);

        CoreGraphics::CmdPushConstants(
            cmdBuf,
            CoreGraphics::GraphicsPipeline,
            textureConstant,
            sizeof(IndexT), &textureId
        );

        // setup primitive
        CoreGraphics::PrimitiveGroup primitive;
        primitive.SetNumIndices(0);
        primitive.SetBaseIndex(0);
        primitive.SetBaseVertex(0);
        primitive.SetNumVertices(batch.vertices.Size());

        // prepare render device and draw
        CoreGraphics::CmdDraw(cmdBuf, primitive);

        CoreGraphics::BufferFlush(vertexBuffer);
    }
}

} // namespace TBUI
