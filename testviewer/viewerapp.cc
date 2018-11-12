//------------------------------------------------------------------------------
// viewerapp.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "visibility/visibilitycontext.h"
#include "models/streammodelpool.h"
#include "models/modelcontext.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "viewerapp.h"
#include "math/vector.h"
#include "math/point.h"
#include "dynui/imguicontext.h"
#include "imgui.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/im3d/im3d.h"

using namespace Timing;
using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace Tests
{

const char* stateToString(Resources::Resource::State state)
{
    switch (state)
    {
    case Resources::Resource::State::Pending: return "Pending";
    case Resources::Resource::State::Loaded: return "Loaded";
    case Resources::Resource::State::Failed: return "Failed";
    case Resources::Resource::State::Unloaded: return "Unloaded";
    }
    return "Unknown";
}





SimpleViewerApplication::SimpleViewerApplication()
{
    this->SetAppTitle("Viewer App");
    this->SetCompanyName("Nebula");
}

SimpleViewerApplication::~SimpleViewerApplication()
{
    // empty
}

bool SimpleViewerApplication::Open()
{
    if (Application::Open())
    {
        this->gfxServer = GraphicsServer::Create();
        this->resMgr = Resources::ResourceManager::Create();
        this->inputServer = Input::InputServer::Create();
        this->ioServer = IO::IoServer::Create();

        this->resMgr->Open();
        this->inputServer->Open();
        this->gfxServer->Open();

        SizeT width = this->GetCmdLineArgs().GetInt("-w", 1024);
        SizeT height = this->GetCmdLineArgs().GetInt("-h", 768);
        

        CoreGraphics::WindowCreateInfo wndInfo =
        {
            CoreGraphics::DisplayMode{ 100, 100, width, height },
            this->GetAppTitle(), "", CoreGraphics::AntiAliasQuality::None, true, true, false
        };
        this->wnd = CreateWindow(wndInfo);

        // create contexts, this could and should be bundled together
        CameraContext::Create();
        ModelContext::Create();
        ObserverContext::Create();
        ObservableContext::Create();
        Dynui::ImguiContext::Create();
        Im3d::Im3dContext::Create();

        Im3d::Im3dContext::SetGridStatus(true);
        Im3d::Im3dContext::SetGridSize(1.0f, 25);
        Im3d::Im3dContext::SetGridColor(Math::float4(0.2f, 0.2f, 0.2f, 0.8f));

        this->view = gfxServer->CreateView("mainview", "frame:vkdebug.json");
        this->stage = gfxServer->CreateStage("stage1", true);
        this->cam = Graphics::CreateEntity();
        CameraContext::RegisterEntity(this->cam);
        CameraContext::SetupProjectionFov(this->cam, width / (float)height, 45.f, 0.01f, 1000.0f);


        this->defaultViewPoint = Math::point(15.0f, 15.0f, -15.0f);

        this->ResetCamera();
        CameraContext::SetTransform(this->cam, this->mayaCameraUtil.GetCameraTransform());

        this->view->SetCamera(this->cam);
        this->view->SetStage(this->stage);

        this->entity = Graphics::CreateEntity();
        ModelContext::RegisterEntity(this->entity);
        ModelContext::Setup(this->entity, "mdl:Buildings/castle_tower.n3", "Viewer");
        ModelContext::SetTransform(this->entity, Math::matrix44::translation(Math::float4(0, 0, 0, 1)));

        // register visibility system
        ObserverContext::CreateBruteforceSystem({});

        ObservableContext::RegisterEntity(this->entity);
        ObservableContext::Setup(this->entity, VisibilityEntityType::Model);
        ObserverContext::RegisterEntity(this->cam);
        ObserverContext::Setup(this->cam, VisibilityEntityType::Camera);



        return true;
    }
    return false;
}
void SimpleViewerApplication::Close()
{
    DestroyWindow(this->wnd);
    this->gfxServer->DiscardStage(this->stage);
    this->gfxServer->DiscardView(this->view);

    this->gfxServer->Close();
    this->inputServer->Close();
    this->resMgr->Close();
    this->Close();
}

void SimpleViewerApplication::Run()
{    
    bool run = true;

    const Ptr<Input::Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();
    
    
    while (run && !inputServer->IsQuitRequested())
    {                     
        this->inputServer->BeginFrame();
        this->inputServer->OnFrame();

        this->resMgr->Update(frameIndex);

        this->gfxServer->BeginFrame();
        this->RenderUI();
        // put game code which doesn't need visibility data or animation here
        this->gfxServer->BeforeViews();

        Im3d::Mat4 trans = ModelContext::GetTransform(this->entity);
        auto const& cts = Im3d::GetContext();
        bool updateCam = true;
        if (Im3d::Gizmo("GizmoUnified", trans))
        {
            updateCam = false;
            ModelContext::SetTransform(this->entity, trans);
        }


        // put game code which need visibility data here

        this->gfxServer->RenderViews();

        // put game code which needs rendering to be done (animation etc) here

        this->gfxServer->EndViews();

        
        // do stuff after rendering is done

        this->gfxServer->EndFrame();

        // force wait immediately
        WindowPresent(wnd, frameIndex);
        if (this->inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::Escape)) run = false;        
        
        if(updateCam)
            this->UpdateCamera();
        

        frameIndex++;             
        this->inputServer->EndFrame();
    }
}
void SimpleViewerApplication::RenderUI()
{
    ImGui::Begin("Viewer", nullptr, ImVec2(240, 400), 0.25, 0);
    if (ImGui::CollapsingHeader("Camera mode","camMode",true, true))    
    {
        if (ImGui::RadioButton("Maya", &this->cameraMode, 0))this->ToMaya();
        ImGui::SameLine();
        if (ImGui::RadioButton("Free", &this->cameraMode, 1))this->ToFree();
        ImGui::SameLine();
        if (ImGui::Button("Reset")) this->ResetCamera();
    }
    Models::ModelId model = ModelContext::GetModel(this->entity);
    auto modelPool = Resources::GetStreamPool<Models::StreamModelPool>();
    auto resource = modelPool->GetName(model);    
    ImGui::Separator();
    ImGui::Text("Resource: %s", resource.AsString().AsCharPtr());
    ImGui::Text("State: %s", stateToString(modelPool->GetState(model)));
    if (ImGui::Button("Browse"))
    {
        ImGui::OpenPopup("Browse for Model");        
        this->Browse();
    }
    if (ImGui::BeginPopupModal("Browse for Model"))
    {
        ImGui::BeginChild("##browserheader", ImVec2(0, 300), true);// ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y));
        ImGui::Columns(2);
        ImGui::Text("Folder");
        for (int i = 0; i < this->folders.Size(); i++)
        {
            if (ImGui::Selectable(this->folders[i].AsCharPtr(), i == this->selectedFolder))
            {
                this->selectedFolder = i;
                this->files = IO::IoServer::Instance()->ListFiles("mdl:" + this->folders[i], "*");
            }
        }
        ImGui::NextColumn();
        ImGui::Text("Files");
            
        for (int i = 0; i < this->files.Size(); i++)
        {
            if (ImGui::Selectable(this->files[i].AsCharPtr(), i == this->selectedFile))
            {
                this->selectedFile = i;                    
            }
        }
        ImGui::EndChild();
        if (ImGui::Button("OK",ImVec2(120, 40))) 
        {
            ImGui::CloseCurrentPopup(); 
            Util::String file = "mdl:" + this->folders[this->selectedFolder] + "/" + this->files[this->selectedFile];                                   
            ModelContext::ChangeModel(this->entity, file, "Viewer");
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel",ImVec2(120, 40))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }            
    ImGui::End();
}
void SimpleViewerApplication::UpdateCamera()
{
    const Ptr<Input::Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();

    this->mayaCameraUtil.SetOrbitButton(mouse->ButtonPressed(Input::MouseButton::LeftButton));
    this->mayaCameraUtil.SetPanButton(mouse->ButtonPressed(Input::MouseButton::MiddleButton));
    this->mayaCameraUtil.SetZoomButton(mouse->ButtonPressed(Input::MouseButton::RightButton));
    this->mayaCameraUtil.SetZoomInButton(mouse->WheelForward());
    this->mayaCameraUtil.SetZoomOutButton(mouse->WheelBackward());
    this->mayaCameraUtil.SetMouseMovement(mouse->GetMovement());

    // process keyboard input
    Math::float4 pos(0.0f);
    if (keyboard->KeyDown(Input::Key::Space))
    {
        this->mayaCameraUtil.Reset();
    }
    if (keyboard->KeyPressed(Input::Key::Left))
    {
        panning.x() -= 0.1f;
        pos.x() -= 0.1f;
    }
    if (keyboard->KeyPressed(Input::Key::Right))
    {
        panning.x() += 0.1f;
        pos.x() += 0.1f;
    }
    if (keyboard->KeyPressed(Input::Key::Up))
    {
        panning.y() -= 0.1f;
        if (keyboard->KeyPressed(Input::Key::LeftShift))
        {
            pos.y() -= 0.1f;
        }
        else
        {
            pos.z() -= 0.1f;
        }
    }
    if (keyboard->KeyPressed(Input::Key::Down))
    {
        panning.y() += 0.1f;
        if (keyboard->KeyPressed(Input::Key::LeftShift))
        {
            pos.y() += 0.1f;
        }
        else
        {
            pos.z() += 0.1f;
        }
    }


    this->mayaCameraUtil.SetPanning(panning);
    this->mayaCameraUtil.SetOrbiting(orbiting);
    this->mayaCameraUtil.SetZoomIn(zoomIn);
    this->mayaCameraUtil.SetZoomOut(zoomOut);
    this->mayaCameraUtil.Update();

    
    this->freeCamUtil.SetForwardsKey(keyboard->KeyPressed(Input::Key::W));
    this->freeCamUtil.SetBackwardsKey(keyboard->KeyPressed(Input::Key::S));
    this->freeCamUtil.SetRightStrafeKey(keyboard->KeyPressed(Input::Key::D));
    this->freeCamUtil.SetLeftStrafeKey(keyboard->KeyPressed(Input::Key::A));
    this->freeCamUtil.SetUpKey(keyboard->KeyPressed(Input::Key::Q));
    this->freeCamUtil.SetDownKey(keyboard->KeyPressed(Input::Key::E));

    this->freeCamUtil.SetMouseMovement(mouse->GetMovement());
    this->freeCamUtil.SetAccelerateButton(keyboard->KeyPressed(Input::Key::LeftShift));

    this->freeCamUtil.SetRotateButton(mouse->ButtonPressed(Input::MouseButton::LeftButton));
    this->freeCamUtil.Update();
    
    switch (this->cameraMode)
    {
    case 0:
        CameraContext::SetTransform(this->cam, Math::matrix44::inverse(this->mayaCameraUtil.GetCameraTransform()));
        break;
    case 1:
        CameraContext::SetTransform(this->cam, Math::matrix44::inverse(this->freeCamUtil.GetTransform()));
        break;
    default:
        break;
    }
}
void SimpleViewerApplication::ResetCamera()
{
    this->freeCamUtil.Setup(this->defaultViewPoint, Math::float4::normalize(this->defaultViewPoint));
    this->freeCamUtil.Update();
    this->mayaCameraUtil.Setup(Math::point(0.0f, 0.0f, 0.0f), this->defaultViewPoint, Math::vector(0.0f, 1.0f, 0.0f));
}
void SimpleViewerApplication::ToMaya()
{
    this->mayaCameraUtil.Setup(this->mayaCameraUtil.GetCenterOfInterest(), this->freeCamUtil.GetTransform().get_position(), Math::vector(0, 1, 0));
}
void SimpleViewerApplication::ToFree()
{
    Math::float4 pos = this->mayaCameraUtil.GetCameraTransform().get_position();
    this->freeCamUtil.Setup(pos, Math::float4::normalize(pos - this->mayaCameraUtil.GetCenterOfInterest()));
}
void SimpleViewerApplication::Browse()
{
    this->folders = IO::IoServer::Instance()->ListDirectories("mdl:", "*");    
    this->files = IO::IoServer::Instance()->ListFiles("mdl:" + this->folders[this->selectedFolder], "*");
}
}
