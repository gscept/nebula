//------------------------------------------------------------------------------
//  im3dcontext.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "im3dcontext.h"
#include "im3d.h"
#include "graphics/graphicsserver.h"
#include "math/rectangle.h"
#include "util/bit.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/pipeline.h"
#include "coregraphics/shader.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "frame/framesubgraph.h"
#include "frame/framecode.h"
#include "imgui.h"

#include "frame/default.h"

using namespace Math;
using namespace CoreGraphics;
using namespace Graphics;
using namespace Base;
using namespace Input;

namespace Im3d
{

class Im3dInputHandler : public Input::InputHandler
{
    __DeclareClass(Im3dInputHandler);
public:
    /// constructor
    Im3dInputHandler() {}
    /// destructor
    virtual ~Im3dInputHandler() {}

    /// capture input to this event handler
    virtual void BeginCapture()
    {
        Input::InputServer::Instance()->ObtainMouseCapture(this);
        Input::InputServer::Instance()->ObtainKeyboardCapture(this);
    }
    /// end input capturing to this event handler
    virtual void EndCapture()
    {
        Input::InputServer::Instance()->ReleaseMouseCapture(this);
        Input::InputServer::Instance()->ReleaseKeyboardCapture(this);
    }

protected:

    /// called when an input event should be processed
    virtual bool OnEvent(const Input::InputEvent& inputEvent)
    {
        switch (inputEvent.GetType())
        {
#ifndef _DEBUG
        case Input::InputEvent::AppObtainFocus:
        case Input::InputEvent::AppLoseFocus:
#endif
        case Input::InputEvent::Reset:
            this->OnReset();
            break;

        default:
            return Im3dContext::HandleInput(inputEvent);
        }
        return false;
    }

private:
    
};

__ImplementClass(Im3d::Im3dInputHandler, 'IM3H', Input::InputHandler);


struct Im3dState
{
    CoreGraphics::ShaderId im3dShader;
    CoreGraphics::ShaderProgramId lines;
    CoreGraphics::ShaderProgramId depthLines;
    CoreGraphics::ShaderProgramId points;
    CoreGraphics::ShaderProgramId triangles;
    CoreGraphics::ShaderProgramId depthTriangles;

    CoreGraphics::PipelineId linesPipeline;
    CoreGraphics::PipelineId depthLinesPipeline;
    CoreGraphics::PipelineId pointsPipeline;
    CoreGraphics::PipelineId trianglesPipeline;
    CoreGraphics::PipelineId depthTrianglesPipeline;

    CoreGraphics::BufferId vbo;     
    CoreGraphics::VertexLayoutId vlo;
    bool renderGrid = false;
    int gridSize = 20;
    float cellSize = 1.0f;
    Math::vec4 gridColor{ 1.0f,1.0f,1.0f,0.3f };
    Math::vec2 gridOffset{ 0, 0 };
    Ptr<Im3dInputHandler> inputHandler;
    Im3d::Id depthLayerId;
    byte* vertexPtr;
    Math::vec2 viewPortPosition{ 0.0f, 0.0f };
    Math::vec2 viewPortSize{ 1.0f, 1.0f };
};
static Im3dState imState;

__ImplementPluginContext(Im3dContext);
//------------------------------------------------------------------------------
/**
*/
Im3dContext::Im3dContext()
{
    //empty;
}

//------------------------------------------------------------------------------
/**
*/
Im3dContext::~Im3dContext()
{
    //empty
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::Create()
{
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    imState.inputHandler = Im3dInputHandler::Create();
    Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, imState.inputHandler.upcast<Input::InputHandler>());

    // allocate im3d shader
    imState.im3dShader = CoreGraphics::ShaderGet("shd:im3d/shaders/im3d.gplb");
    imState.lines = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureMask("Static|Lines"));
    imState.depthLines = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureMask("StaticDepth|Lines"));
    imState.points = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureMask("Static|Points"));
    imState.triangles = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureMask("Static|Triangles"));
    imState.depthTriangles = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureMask("StaticDepth|Triangles"));
    
    imState.depthLayerId = Im3d::MakeId("depthEnabled");
    // create vertex buffer
    Util::Array<CoreGraphics::VertexComponent> components;
    components.Append(VertexComponent(0, VertexComponent::Float4, 0));
    components.Append(VertexComponent(1, VertexComponent::UByte4N, 0));
    imState.vlo = CreateVertexLayout({ .name = "Im3D"_atm, .comps = components });

    CoreGraphics::BufferCreateInfo vboInfo;
    vboInfo.name = "Im3D VBO"_atm;
    vboInfo.size = 100000 * 3;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(imState.vlo);
    vboInfo.mode = CoreGraphics::HostCached;
    vboInfo.usageFlags = CoreGraphics::BufferUsage::Vertex;
    vboInfo.data = nullptr;
    vboInfo.dataSize = 0;
    imState.vbo = CoreGraphics::CreateBuffer(vboInfo);

    // map buffer
    imState.vertexPtr = (byte*)CoreGraphics::BufferMap(imState.vbo);

    FrameScript_default::RegisterSubgraph_Im3D_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        Render(cmdBuf, frame);
    });
    FrameScript_default::RegisterSubgraphPipelines_Im3D_Pass([](const CoreGraphics::PassId pass, const uint subpass)
    {
        if (imState.linesPipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(imState.linesPipeline);
        imState.linesPipeline = CoreGraphics::CreateGraphicsPipeline({ imState.lines, pass, subpass, CoreGraphics::InputAssemblyKey{ CoreGraphics::PrimitiveTopology::LineList, false } });

        if (imState.depthLinesPipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(imState.depthLinesPipeline);
        imState.depthLinesPipeline = CoreGraphics::CreateGraphicsPipeline({ imState.depthLines, pass, subpass, CoreGraphics::InputAssemblyKey{ CoreGraphics::PrimitiveTopology::LineList, false } });

        if (imState.trianglesPipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(imState.trianglesPipeline);
        imState.trianglesPipeline = CoreGraphics::CreateGraphicsPipeline({ imState.triangles, pass, subpass, CoreGraphics::InputAssemblyKey{ CoreGraphics::PrimitiveTopology::TriangleList, false } });

        if (imState.depthTrianglesPipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(imState.depthTrianglesPipeline);
        imState.depthTrianglesPipeline = CoreGraphics::CreateGraphicsPipeline({ imState.depthTriangles, pass, subpass, CoreGraphics::InputAssemblyKey{ CoreGraphics::PrimitiveTopology::TriangleList, false } });

        if (imState.pointsPipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(imState.pointsPipeline);
        imState.pointsPipeline = CoreGraphics::CreateGraphicsPipeline({ imState.points, pass, subpass, CoreGraphics::InputAssemblyKey{ CoreGraphics::PrimitiveTopology::PointList, false } });
    });
    Im3d::NewFrame();

}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::Discard()
{
    Input::InputServer::Instance()->RemoveInputHandler(imState.inputHandler.upcast<InputHandler>());
    imState.inputHandler = nullptr;
    CoreGraphics::BufferUnmap(imState.vbo);

    CoreGraphics::DestroyBuffer(imState.vbo);
    imState.vertexPtr = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawText(const Math::vec3& position, Util::String const& name, const float size, const Math::vec4 color, uint32_t renderFlags)
{
    if (renderFlags & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::Text(position, size, Im3d::Vec4(color), Im3d::TextFlags::TextFlags_Default, name.AsCharPtr());
    if (renderFlags & CheckDepth) Im3d::PopLayerId();
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawPoint(const Math::vec3& position, const float size, const Math::vec4 color, uint32_t renderFlags)
{
    if (renderFlags & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::DrawPoint(Im3d::Vec3(position), size, Im3d::Vec4(color));
    if (renderFlags & CheckDepth) Im3d::PopLayerId();
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawLine(const Math::line& line, const float size, const Math::vec4 color, uint32_t renderFlags)
{
    if (renderFlags & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::DrawLine(line.start(), line.end(), size, Im3d::Vec4(color));
    if (renderFlags & CheckDepth) Im3d::PopLayerId();
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawQuad(const Math::point& a, const Math::point& b, const Math::point& c, const Math::point& d, const Math::vec4 color, uint32_t renderFlags)
{
    if (renderFlags & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::SetColor(Im3d::Vec4(color));
    if (renderFlags & Wireframe) 
    {
        Im3d::DrawQuad(Im3d::Vec3(a), Im3d::Vec3(b), Im3d::Vec3(c), Im3d::Vec3(d));
    }
    else
    {
        Im3d::DrawQuadFilled(Im3d::Vec3(a), Im3d::Vec3(b), Im3d::Vec3(c), Im3d::Vec3(d));
    }
    if (renderFlags & CheckDepth) Im3d::PopLayerId();
}

//------------------------------------------------------------------------------
/**
*/
void 
Im3dContext::DrawBox(const Math::bbox & box, const Math::vec4 & color, uint32_t depthFlag)
{
    Im3d::PushDrawState();
    if (depthFlag & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::SetSize(1.0f);    
    Im3d::SetColor(Im3d::Vec4(color));
    if (depthFlag &Wireframe)
    {
        Im3d::DrawAlignedBox(box.pmin, box.pmax);
    }
    if (depthFlag & Solid)
    {
        Im3d::DrawAlignedBoxFilled(box.pmin, box.pmax);
    }
    if (depthFlag & CheckDepth) Im3d::PopLayerId();
    Im3d::PopDrawState();
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawBox(const Math::mat4& transform, const Math::vec4 & color, uint32_t depthFlag)
{
    Math::bbox box;
    box.pmin.set(-0.5f, -0.5f, -0.5f);
    box.pmax.set(0.5f, 0.5f, 0.5f);

    DrawOrientedBox(transform, box, color, depthFlag);
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawOrientedBox(const Math::mat4& transform, const Math::bbox & box, const Math::vec4 & color, uint32_t depthFlag)
{
    Im3d::PushDrawState();

    Im3d::Vec3 p[8];
    for (int i = 0; i < 8; i++)
    {
        p[i] = xyz(transform * Math::vec4(box.corner_point(i)));
    }    
    if (depthFlag & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::SetSize(2.0f);
    Im3d::SetColor(Im3d::Vec4(color));
    Im3d::BeginLineLoop();
        Im3d::Vertex(p[0]);
        Im3d::Vertex(p[1]);
        Im3d::Vertex(p[2]);
        Im3d::Vertex(p[3]);
    Im3d::End();
    Im3d::BeginLineLoop();
        Im3d::Vertex(p[4]);
        Im3d::Vertex(p[5]);
        Im3d::Vertex(p[6]);
        Im3d::Vertex(p[7]);
    Im3d::End();
    Im3d::BeginLines();
        Im3d::Vertex(p[0]); Im3d::Vertex(p[6]);
        Im3d::Vertex(p[1]); Im3d::Vertex(p[5]);
        Im3d::Vertex(p[2]); Im3d::Vertex(p[4]);
        Im3d::Vertex(p[3]); Im3d::Vertex(p[7]);
    Im3d::End();
    if (depthFlag & CheckDepth) Im3d::PopLayerId();
    Im3d::PopDrawState();
}


//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawSphere(const Math::mat4& modelTransform, const Math::vec4& color, uint32_t depthFlag)
{
    Im3d::PushDrawState();
    if (depthFlag & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::SetSize(2.0f);    
    Im3d::SetColor(Im3d::Vec4(color));
    Im3d::PushMatrix(modelTransform);
    //Im3d::PushEnableSorting(true);
    if (depthFlag & Wireframe)
    {
        Im3d::DrawSphere(Vec3(0.0f), 1.0f, 16);
    }        
    if (depthFlag & Solid)
    {
        Im3d::DrawSphereFilled(Vec3(0.0f), 1.0f, 64);
    }            
    //Im3d::PopEnableSorting();
    Im3d::PopMatrix();
    if (depthFlag & CheckDepth) Im3d::PopLayerId();
    Im3d::PopDrawState();
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawSphere(const Math::point& pos, float radius, const Math::vec4& color, uint32_t depthFlag)
{
    Im3d::PushDrawState();
    if (depthFlag & CheckDepth) Im3d::PushLayerId(imState.depthLayerId);
    Im3d::SetSize(2.0f);
    Im3d::SetColor(Im3d::Vec4(color));
    if (depthFlag & Wireframe)
    {
        Im3d::DrawSphere(Vec3(pos), radius, 16);
    }
    if (depthFlag & Solid)
    {
        Im3d::DrawSphereFilled(Vec3(pos), radius, 16);
    }
    if (depthFlag & CheckDepth) Im3d::PopLayerId();
    Im3d::PopDrawState();
}

//------------------------------------------------------------------------------
/**
*/
void Im3dContext::DrawCylinder(const Math::point& start, const Math::point& end, float radius, const Math::vec4& color)
{
    Im3d::PushDrawState();
    
    Im3d::SetSize(2.0f);
    Im3d::SetColor(Im3d::Vec4(color));
    Im3d::DrawCylinder(Vec3(start), Vec3(end), radius, 16);
    
    Im3d::PopDrawState();
}

//------------------------------------------------------------------------------
/**
*/
void 
Im3dContext::DrawCone(const Math::point& start, const Math::point& end, float startRadius, float endRadius, const Math::vec4& color, uint32_t renderFlags)
{
    Im3d::PushDrawState();

    Im3d::SetSize(2.0f);
    Im3d::SetColor(Im3d::Vec4(color));
    Im3d::DrawCone2(Vec3(start), Vec3(end), startRadius, endRadius, 16);

    Im3d::PopDrawState();
}
//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    AppData& ad = GetAppData();

    const Math::rectangle<int>& viewport = view->GetViewport();

    ad.m_deltaTime = ctx.frameTime;
    ad.m_viewportSize = Vec2((float)viewport.width(), (float)viewport.height());
    
    Graphics::GraphicsEntityId cam = view->GetCamera();
    Math::mat4 transform = inverse(CameraContext::GetView(cam));
    ad.m_viewOrigin = xyz(transform.position);
    ad.m_viewDirection = -xyz(transform.z_axis);
    ad.m_worldUp = Vec3(0.0f, 1.0f, 0.0f);
    ad.m_projOrtho = false;

    auto const& settings = CameraContext::GetSettings(cam);
    // m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
    ad.m_projScaleY = tanf(settings.GetFov() * 0.5f) * 2.0f; // or vertical fov for a perspective projection
    
    auto const& mouse = Input::InputServer::Instance()->GetDefaultMouse();
    
    // window origin is top-left, ndc is bottom-left
    Math::vec2 mousePos = Math::vec2::divide(mouse->GetPixelPosition() - imState.viewPortPosition, imState.viewPortSize);
    
    mousePos *= 2.0f;
    mousePos -= Math::vec2(1.0f, 1.0f);
    mousePos.y = -mousePos.y;
        
    Vec3 rayOrigin, rayDirection;
    
    auto const& proj = CameraContext::GetProjection(cam);
    auto const& viewProj = CameraContext::GetViewProjection(cam);
    rayOrigin = ad.m_viewOrigin;
    Math::vec4 rayDir(mousePos.x / proj.row0.x, mousePos.y / proj.row1.y, -1.0f, 0.0f);
    rayDir = normalize(rayDir);    
    rayDirection = xyz(transform * rayDir);
    
    ad.m_cursorRayOrigin = rayOrigin;
    ad.m_cursorRayDirection = rayDirection;

    // Set cull frustum planes. This is only required if IM3D_CULL_GIZMOS or IM3D_CULL_PRIMTIIVES is enable via
    // im3d_config.h, or if any of the IsVisible() functions are called.
    ad.setCullFrustum(viewProj, true);

    // Fill the key state array; using GetAsyncKeyState here but this could equally well be done via the window proc.
    // All key states have an equivalent (and more descriptive) 'Action_' enum.

    const Ptr<Input::Keyboard>& keyboard = Input::InputServer::Instance()->GetDefaultKeyboard();    

    ad.m_keyDown[Im3d::Mouse_Left/*Im3d::Action_Select*/] = mouse->ButtonPressed(Input::MouseButton::LeftButton);
    
    // The following key states control which gizmo to use for the generic Gizmo() function. Here using the left ctrl
    // key as an additional predicate.
    bool ctrlDown = keyboard->KeyPressed(Input::Key::LeftControl) || keyboard->KeyPressed(Input::Key::RightControl);
    ad.m_keyDown[Im3d::Key_L/*Action_GizmoLocal*/] = ctrlDown && keyboard->KeyPressed(Input::Key::L);
    ad.m_keyDown[Im3d::Key_T/*Action_GizmoTranslation*/] = ctrlDown && keyboard->KeyPressed(Input::Key::W);
    ad.m_keyDown[Im3d::Key_R/*Action_GizmoRotation*/] = ctrlDown && keyboard->KeyPressed(Input::Key::E);
    ad.m_keyDown[Im3d::Key_S/*Action_GizmoScale*/] = ctrlDown && keyboard->KeyPressed(Input::Key::R);

    // FIXME make these configurable
    // Enable gizmo snapping by setting the translation/rotation/scale increments to be > 0
    ad.m_snapTranslation = ctrlDown ? 0.5f : 0.0f;
    ad.m_snapRotation = ctrlDown ? Math::deg2rad(30.0f) : 0.0f;
    ad.m_snapScale = ctrlDown ? 0.5f : 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
template<typename filterFunc>
static inline void
CollectByFilter(const CoreGraphics::CmdBufferId cmdBuf, const PipelineId pipeline, PrimitiveTopology::Code topology, IndexT &vertexBufferOffset, IndexT & vertexCount, filterFunc && filter)
{
    CoreGraphics::CmdSetGraphicsPipeline(cmdBuf, pipeline);

    // setup input buffers
    CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, topology);
    CoreGraphics::CmdSetVertexLayout(cmdBuf, imState.vlo);
    CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, imState.vbo, 0);

    for (uint32_t i = 0, n = Im3d::GetDrawListCount(); i < n; ++i)
    {
        auto& drawList = Im3d::GetDrawLists()[i];
        if (filter(drawList))
        {

            const unsigned char* vertexBuffer = (unsigned char*)drawList.m_vertexData;
            const SizeT vertexBufferSize = drawList.m_vertexCount * sizeof(Im3d::VertexData);
            Memory::Copy(vertexBuffer, imState.vertexPtr + vertexBufferOffset, vertexBufferSize);

            CoreGraphics::PrimitiveGroup primitive;
            primitive.SetNumIndices(0);
            primitive.SetBaseIndex(0);
            primitive.SetNumVertices(drawList.m_vertexCount);
            primitive.SetBaseVertex(vertexCount);
            CoreGraphics::CmdDraw(cmdBuf, primitive);

            vertexBufferOffset += vertexBufferSize;
            vertexCount += drawList.m_vertexCount;
        }
    }    
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::Render(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex)
{
    if (imState.renderGrid)
    {
        int gridSize = imState.gridSize;            
        float cellSize = imState.cellSize;            
        Im3d::SetSize(1.0f);
        Im3d::PushLayerId(imState.depthLayerId);            
        Im3d::BeginLines();
        Im3d::Color col(imState.gridColor.x, imState.gridColor.y,imState.gridColor.z,imState.gridColor.w);
        for (int x = -gridSize; x <= gridSize; ++x) {
            Im3d::Vertex(-gridSize * cellSize + imState.gridOffset.x, 0.0f, (float)x * cellSize + imState.gridOffset.y, col);
            Im3d::Vertex(gridSize * cellSize + imState.gridOffset.x, 0.0f, (float)x * cellSize + imState.gridOffset.y, col);
        }
        for (int z = -gridSize; z <= gridSize; ++z) {
            Im3d::Vertex((float)z * cellSize + imState.gridOffset.x, 0.0f, -gridSize * cellSize + imState.gridOffset.y, col);
            Im3d::Vertex((float)z * cellSize + imState.gridOffset.x, 0.0f, gridSize* cellSize + imState.gridOffset.y, col);
        }
        Im3d::End();
        Im3d::PopLayerId();
    }
    Im3d::EndFrame();

    // setup device
    IndexT vertexCount = 0;
    IndexT vertexBufferOffset = 0;
    // collect draws and loop a couple of times instead

    N_CMD_SCOPE(cmdBuf, NEBULA_MARKER_GRAPHICS, "Im3d");

    CollectByFilter(cmdBuf, imState.pointsPipeline, CoreGraphics::PrimitiveTopology::PointList, vertexBufferOffset, vertexCount,
        [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Points; });
        
    CollectByFilter(cmdBuf, imState.trianglesPipeline, CoreGraphics::PrimitiveTopology::TriangleList, vertexBufferOffset, vertexCount,
        [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Triangles && l.m_layerId != imState.depthLayerId; });
        
    CollectByFilter(cmdBuf, imState.linesPipeline, CoreGraphics::PrimitiveTopology::LineList, vertexBufferOffset, vertexCount,
        [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Lines && l.m_layerId != imState.depthLayerId; });     

    CollectByFilter(cmdBuf, imState.depthLinesPipeline, CoreGraphics::PrimitiveTopology::LineList, vertexBufferOffset, vertexCount,
        [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Lines && l.m_layerId == imState.depthLayerId; });
        
    //CollectByFilter(imState.depthTriangles, CoreGraphics::PrimitiveTopology::TriangleList, vertexBufferOffset, vertexCount,
    CollectByFilter(cmdBuf, imState.depthTrianglesPipeline, CoreGraphics::PrimitiveTopology::TriangleList, vertexBufferOffset, vertexCount,
        [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Triangles && l.m_layerId == imState.depthLayerId; });

    CoreGraphics::BufferFlush(imState.vbo);
    Im3d::NewFrame();
}

//------------------------------------------------------------------------------
/**
*/
bool 
Im3dContext::HandleInput(const Input::InputEvent& event)
{
    auto const& evType = event.GetType();
    auto& ctx = Im3d::GetContext();
    if (evType == InputEvent::MouseButtonDown)
    {
        if (ctx.m_hotId > 0)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::SetGridStatus(bool enable)
{
    imState.renderGrid = enable;
}

//------------------------------------------------------------------------------
/**
*/
bool
Im3dContext::GetGridStatus()
{
    return imState.renderGrid;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::SetGridSize(float cellSize, int cellCount)
{
    imState.cellSize = cellSize;
    imState.gridSize = cellCount;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::GetGridSize(float& cellSize, int& cellCount)
{
    cellSize = imState.cellSize;
    cellCount = imState.gridSize;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::SetGridColor(Math::vec4 const & color)
{
    imState.gridColor = color;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4 const&
Im3dContext::GetGridColor()
{
    return imState.gridColor;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::SetGridOffset(Math::vec2 const& offset)
{
    imState.gridOffset = offset;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2 const&
Im3dContext::GetGridOffset()
{
    return imState.gridOffset;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::SetGizmoSize(int size, int width)
{
    auto& ctx = GetContext();

    ctx.m_gizmoHeightPixels = size;
    ctx.m_gizmoSizePixels = width;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::GetGizmoSize(int& size, int& width)
{
    auto& ctx = GetContext();

    size = ctx.m_gizmoHeightPixels;
    width = ctx.m_gizmoSizePixels;
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::SetViewportRect(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize)
{
    imState.viewPortPosition = viewPortPosition;
    imState.viewPortSize = viewPortSize;
}

} // namespace Im3d
