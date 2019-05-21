//------------------------------------------------------------------------------
//  im3dcontext.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "im3dcontext.h"
#include "im3d.h"
#include "graphics/graphicsserver.h"
#include "math/rectangle.h"
#include "util/bit.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/displaydevice.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "imgui.h"

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
    CoreGraphics::VertexBufferId vbo;       
    bool renderGrid = false;
    int gridSize = 20;
    float cellSize = 1.0f;
    Math::float4 gridColor{ 1.0f,1.0f,1.0f,0.3f };
    Ptr<Im3dInputHandler> inputHandler;
    Im3d::Id depthLayerId;
    byte* vertexPtr;    
};
static Im3dState imState;

_ImplementPluginContext(Im3dContext);
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
    __bundle.OnRenderAsPlugin = Im3dContext::OnRenderAsPlugin;
    __bundle.OnBeforeView = Im3dContext::OnBeforeView;
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);


    imState.inputHandler = Im3dInputHandler::Create();
    //Input::InputServer::Instance()->AttachInputHandler(Input::InputPriority::DynUi, imState.inputHandler.upcast<Input::InputHandler>());

    // allocate imgui shader
    imState.im3dShader = ShaderServer::Instance()->GetShader("shd:im3d.fxb");
    imState.lines = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureFromString("Static|Lines"));
    imState.depthLines = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureFromString("StaticDepth|Lines"));
    imState.points = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureFromString("Static|Points"));
    imState.triangles = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureFromString("Static|Triangles"));
    imState.depthTriangles = CoreGraphics::ShaderGetProgram(imState.im3dShader, CoreGraphics::ShaderFeatureFromString("StaticDepth|Triangles"));
    
    imState.depthLayerId = Im3d::MakeId("depthEnabled");
    // create vertex buffer
    Util::Array<CoreGraphics::VertexComponent> components;
    components.Append(VertexComponent((VertexComponent::SemanticName)0, 0, VertexComponentBase::Float4, 0));
    components.Append(VertexComponent((VertexComponent::SemanticName)1, 0, VertexComponentBase::UByte4N, 0));

    CoreGraphics::VertexBufferCreateInfo vboInfo =
    {
        "im3d_vbo"_atm,
        "system",
        CoreGraphics::GpuBufferTypes::AccessWrite,
        CoreGraphics::GpuBufferTypes::UsageDynamic,
        CoreGraphics::GpuBufferTypes::SyncingCoherent | CoreGraphics::GpuBufferTypes::SyncingPersistent,
        100000 * 3,
        components,
        nullptr,
        0
    };
    imState.vbo = CoreGraphics::CreateVertexBuffer(vboInfo);

    // map buffer
    imState.vertexPtr = (byte*)CoreGraphics::VertexBufferMap(imState.vbo, CoreGraphics::GpuBufferTypes::MapWrite);
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::Discard()
{
    Input::InputServer::Instance()->RemoveInputHandler(imState.inputHandler.upcast<InputHandler>());
    imState.inputHandler = nullptr;
    CoreGraphics::VertexBufferUnmap(imState.vbo);

    CoreGraphics::DestroyVertexBuffer(imState.vbo);
    imState.vertexPtr = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
Im3dContext::DrawBox(const Math::bbox & box, const Math::float4 & color, uint32_t depthFlag)
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
Im3dContext::DrawBox(const Math::matrix44& transform, const Math::float4 & color, uint32_t depthFlag)
{
    Math::bbox box;
    box.pmin.set(-1.0f, -1.0f, -1.0f);
    box.pmax.set(1.0f, 1.0f, 1.0f);

    DrawOrientedBox(transform, box, color, depthFlag);
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::DrawOrientedBox(const Math::matrix44& transform, const Math::bbox & box, const Math::float4 & color, uint32_t depthFlag)
{
    Im3d::PushDrawState();

    Im3d::Vec3 p[8];
    for (int i = 0; i < 8; i++)
    {
        p[i] = Math::matrix44::transform(box.corner_point(i), transform);
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
Im3dContext::DrawSphere(const Math::matrix44& modelTransform, const Math::float4& color, uint32_t depthFlag)
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
Im3dContext::OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
    AppData& ad = GetAppData();

    ad.m_deltaTime = frameTime;
    SetGizmoSize(128, 4);
    auto const & mode = CoreGraphics::WindowGetDisplayMode(DisplayDevice::Instance()->GetCurrentWindow());
    ad.m_viewportSize = Vec2((float)mode.GetWidth(), (float)mode.GetHeight());
    
    Graphics::GraphicsEntityId cam = view->GetCamera();
    Math::matrix44 transform = Math::matrix44::inverse(CameraContext::GetTransform(cam));
    ad.m_viewOrigin = transform.get_position();
    ad.m_viewDirection = -transform.get_zaxis();
    ad.m_worldUp = Vec3(0.0f, 1.0f, 0.0f);
    ad.m_projOrtho = false;

    auto const& settings = CameraContext::GetSettings(cam);
    // m_projScaleY controls how gizmos are scaled in world space to maintain a constant screen height
    ad.m_projScaleY = tanf(Math::n_deg2rad(settings.GetFov()) * 0.5f) * 2.0f; // or vertical fov for a perspective projection
    
    auto const& mouse = Input::InputServer::Instance()->GetDefaultMouse();
    
    // window origin is top-left, ndc is bottom-left
    Math::float2 mousePos = mouse->GetScreenPosition();
    mousePos *= 2.0f;
    mousePos -= Math::float2(1.0f, 1.0f);
    mousePos.y() = -mousePos.y();
        
    Vec3 rayOrigin, rayDirection;
    
    auto const& proj = CameraContext::GetProjection(cam);
    auto const& viewProj = CameraContext::GetViewProjection(cam);
    rayOrigin = ad.m_viewOrigin;
    Math::float4 rayDir(mousePos.x() / proj.getrow0().x(), mousePos.y() / proj.getrow1().y(), -1.0f, 0.0f);
    rayDir = Math::float4::normalize(rayDir);    
    rayDirection = matrix44::transform(rayDir, transform);
    
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
    ad.m_keyDown[Im3d::Key_L/*Action_GizmoLocal*/] = ctrlDown && (GetAsyncKeyState(0x4c) & 0x8000) != 0;
    ad.m_keyDown[Im3d::Key_T/*Action_GizmoTranslation*/] = keyboard->KeyPressed(Input::Key::W);
    ad.m_keyDown[Im3d::Key_R/*Action_GizmoRotation*/] = keyboard->KeyPressed(Input::Key::E);
    ad.m_keyDown[Im3d::Key_S/*Action_GizmoScale*/] = keyboard->KeyPressed(Input::Key::R);

    // FIXME make these configurable
    // Enable gizmo snapping by setting the translation/rotation/scale increments to be > 0
    ad.m_snapTranslation = ctrlDown ? 0.5f : 0.0f;
    ad.m_snapRotation = ctrlDown ? Math::n_deg2rad(30.0f) : 0.0f;
    ad.m_snapScale = ctrlDown ? 0.5f : 0.0f;
    Im3d::NewFrame();
}

//------------------------------------------------------------------------------
/**
*/
template<typename filterFunc>
static inline 
void
CollectByFilter(ShaderProgramId const & shader, PrimitiveTopology::Code topology, IndexT &vertexBufferOffset, IndexT & vertexCount, filterFunc && filter)
{
    CoreGraphics::SetShaderProgram(shader);
    CoreGraphics::SetPrimitiveTopology(topology);
	CoreGraphics::SetVertexLayout(CoreGraphics::VertexBufferGetLayout(imState.vbo));

    CoreGraphics::SetGraphicsPipeline();
    // setup input buffers
    CoreGraphics::SetStreamVertexBuffer(0, imState.vbo, 0);    

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
            CoreGraphics::SetPrimitiveGroup(primitive);            
            CoreGraphics::Draw();

            vertexBufferOffset += vertexBufferSize;
            vertexCount += drawList.m_vertexCount;
        }
    }    
}

//------------------------------------------------------------------------------
/**
*/
void
Im3dContext::OnRenderAsPlugin(const IndexT frameIndex, const Timing::Time frameTime, const Util::StringAtom& filter)
{
    //FIME use a better pass
    if (filter == "IMGUI"_atm)
    {
     
        if (imState.renderGrid)
        {
            int gridSize = imState.gridSize;            
            float cellSize = imState.cellSize;            
            Im3d::SetSize(1.0f);
            Im3d::PushLayerId(imState.depthLayerId);            
            Im3d::BeginLines();
            Im3d::Color col = imState.gridColor;
            for (int x = -gridSize; x <= gridSize; ++x) {
                Im3d::Vertex(-gridSize * cellSize, 0.0f, (float)x * cellSize, col);
                Im3d::Vertex(gridSize * cellSize, 0.0f, (float)x * cellSize, col);
            }
            for (int z = -gridSize; z <= gridSize; ++z) {
                Im3d::Vertex((float)z * cellSize, 0.0f, -gridSize * cellSize, col);
                Im3d::Vertex((float)z * cellSize, 0.0f, gridSize* cellSize, col);
            }
            Im3d::End();
            Im3d::PopLayerId();
        }
        Im3d::EndFrame();
        VertexBufferId vbo = imState.vbo;                

        // setup device
        IndexT vertexCount = 0;
        IndexT vertexBufferOffset = 0;
        // collect draws and loop a couple of times instead

        CollectByFilter(imState.points, CoreGraphics::PrimitiveTopology::PointList, vertexBufferOffset, vertexCount,
            [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Points; });
        
        CollectByFilter(imState.triangles, CoreGraphics::PrimitiveTopology::TriangleList, vertexBufferOffset, vertexCount,
            [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Triangles && l.m_layerId != imState.depthLayerId; });
        
        CollectByFilter(imState.lines, CoreGraphics::PrimitiveTopology::LineList, vertexBufferOffset, vertexCount,
            [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Lines && l.m_layerId != imState.depthLayerId; });     

        CollectByFilter(imState.depthLines, CoreGraphics::PrimitiveTopology::LineList, vertexBufferOffset, vertexCount,
            [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Lines && l.m_layerId == imState.depthLayerId; });
        
        //CollectByFilter(imState.depthTriangles, CoreGraphics::PrimitiveTopology::TriangleList, vertexBufferOffset, vertexCount,
        CollectByFilter(imState.depthTriangles, CoreGraphics::PrimitiveTopology::TriangleList, vertexBufferOffset, vertexCount,
            [](Im3d::DrawList const& l) { return l.m_primType == Im3d::DrawPrimitive_Triangles && l.m_layerId == imState.depthLayerId; });
    }
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
Im3dContext::SetGridColor(Math::float4 const & color)
{
    imState.gridColor = color;
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

}
