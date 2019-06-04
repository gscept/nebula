//------------------------------------------------------------------------------
// physicsapp.cc
// (C) 2019 Individual contributors, see AUTHORS file
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
#include "physicsapp.h"
#include "math/vector.h"
#include "math/point.h"
#include "dynui/imguicontext.h"
#include "lighting/lightcontext.h"
#include "characters/charactercontext.h"
#include "imgui.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/im3d/im3d.h"
#include "physics/actorcontext.h"
#include "physics/streamactorpool.h"
#include "physics/utils.h"

using namespace Timing;
using namespace Graphics;
using namespace Visibility;
using namespace Models;

namespace Tests
{

//------------------------------------------------------------------------------
/**
*/
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

//------------------------------------------------------------------------------
/**
*/
SimpleViewerApplication::SimpleViewerApplication() : fpsGraph("FPS", 512)
{
    this->SetAppTitle("Physics App");
    this->SetCompanyName("Nebula");
}

//------------------------------------------------------------------------------
/**
*/
SimpleViewerApplication::~SimpleViewerApplication()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool 
SimpleViewerApplication::Open()
{
    if (Application::Open())
    {
        this->gfxServer = GraphicsServer::Create();
        this->resMgr = Resources::ResourceManager::Create();
        this->inputServer = Input::InputServer::Create();
        this->ioServer = IO::IoServer::Create();
        this->debugInterface = Debug::DebugInterface::Create();

        this->debugInterface->Open();

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
		Lighting::LightContext::Create();
		Characters::CharacterContext::Create();
        Dynui::ImguiContext::Create();
        Im3d::Im3dContext::Create();
        this->fpsGraph.SetLimits(20.0f, 500.0f);

        Physics::Setup();
        this->physicsScene = Physics::CreateScene();
        IndexT dummyMaterial = Physics::CreateMaterial("dummy"_atm, 0.8, 0.6, 0.3, 1.0);

        Im3d::Im3dContext::SetGridStatus(true);
        Im3d::Im3dContext::SetGridSize(1.0f, 25);
        Im3d::Im3dContext::SetGridColor(Math::float4(0.2f, 0.2f, 0.2f, 0.8f));

        this->view = gfxServer->CreateView("mainview", "frame:vkdebug.json");
        this->stage = gfxServer->CreateStage("stage1", true);
        this->cam = Graphics::CreateEntity();
        CameraContext::RegisterEntity(this->cam);
        CameraContext::SetupProjectionFov(this->cam, width / (float)height, 45.f, 0.01f, 1000.0f);

		this->globalLight = Graphics::CreateEntity();
		Lighting::LightContext::RegisterEntity(this->globalLight);
		Lighting::LightContext::SetupGlobalLight(this->globalLight, Math::float4(1, 1, 1, 0), 1.0f, Math::float4(0, 0, 0, 0), Math::float4(0, 0, 0, 0), 0.0f, Math::vector(1, 1, 1), false);

		this->pointLights[0] = Graphics::CreateEntity();
		Lighting::LightContext::RegisterEntity(this->pointLights[0]);
		Lighting::LightContext::SetupPointLight(this->pointLights[0], Math::float4(1, 0, 0, 1), 10.0f, Math::matrix44::translation(0, 0, -10), 10.0f, false);
        
		this->pointLights[1] = Graphics::CreateEntity();
		Lighting::LightContext::RegisterEntity(this->pointLights[1]);
		Lighting::LightContext::SetupPointLight(this->pointLights[1], Math::float4(0, 1, 0, 1), 10.0f, Math::matrix44::translation(-10, 0, -10), 10.0f, false);

		this->pointLights[2] = Graphics::CreateEntity();
		Lighting::LightContext::RegisterEntity(this->pointLights[2]);
		Lighting::LightContext::SetupPointLight(this->pointLights[2], Math::float4(0, 0, 1, 1), 10.0f, Math::matrix44::translation(-10, 0, 0), 10.0f, false);

        for (int i = 0; i < 3; i++)
        {
            this->entities.Append(this->pointLights[i]);
        }

		{
			this->spotLights[0] = Graphics::CreateEntity();
			Lighting::LightContext::RegisterEntity(this->spotLights[0]);
			Math::matrix44 spotLightMatrix;
			spotLightMatrix.scale(Math::vector(30, 30, 40));
			spotLightMatrix = Math::matrix44::multiply(spotLightMatrix, Math::matrix44::rotationyawpitchroll(0, Math::n_deg2rad(-55), 0));
			spotLightMatrix.set_position(Math::point(0, 5, 2));
			Lighting::LightContext::SetupSpotLight(this->spotLights[0], Math::float4(1, 1, 0, 1), 1.0f, spotLightMatrix, false);
		}

		{
			this->spotLights[1] = Graphics::CreateEntity();
			Lighting::LightContext::RegisterEntity(this->spotLights[1]);
			Math::matrix44 spotLightMatrix;
			spotLightMatrix.scale(Math::vector(30, 30, 40));
			spotLightMatrix = Math::matrix44::multiply(spotLightMatrix, Math::matrix44::rotationyawpitchroll(Math::n_deg2rad(60), Math::n_deg2rad(-55), 0));
			spotLightMatrix.set_position(Math::point(2, 5, 0));
			Lighting::LightContext::SetupSpotLight(this->spotLights[1], Math::float4(0, 1, 1, 1), 1.0f, spotLightMatrix, false);
		}

		{
			this->spotLights[2] = Graphics::CreateEntity();
			Lighting::LightContext::RegisterEntity(this->spotLights[2]);
			Math::matrix44 spotLightMatrix;
			spotLightMatrix.scale(Math::vector(30, 30, 40));
			spotLightMatrix = Math::matrix44::multiply(spotLightMatrix, Math::matrix44::rotationyawpitchroll(Math::n_deg2rad(120), Math::n_deg2rad(-55), 0));
			spotLightMatrix.set_position(Math::point(2, 5, 2));
			Lighting::LightContext::SetupSpotLight(this->spotLights[2], Math::float4(1, 0, 1, 1), 1.0f, spotLightMatrix, false);
		}

        this->defaultViewPoint = Math::point(15.0f, 15.0f, -15.0f);
        this->ResetCamera();
        CameraContext::SetTransform(this->cam, this->mayaCameraUtil.GetCameraTransform());

        this->view->SetCamera(this->cam);
        this->view->SetStage(this->stage);

        this->entity = Graphics::CreateEntity();
        ModelContext::RegisterEntity(this->entity);        
        ModelContext::Setup(this->entity, "mdl:Units/Unit_Archer.n3", "Viewer");
        ModelContext::SetTransform(this->entity, Math::matrix44::translation(Math::float4(0, 0, 0, 1)));
        this->entities.Append(this->entity);

		this->ground = Graphics::CreateEntity();
		ModelContext::RegisterEntity(this->ground);
		ModelContext::Setup(this->ground, "mdl:environment/Groundplane.n3", "Viewer");
		ModelContext::SetTransform(this->ground, Math::matrix44::translation(Math::float4(0, 0, 0, 1)));
        this->entities.Append(this->ground);

        Resources::ResourceId ground = Resources::CreateResource("phy:test/groundplane.np", "Viewer", nullptr, nullptr, true);        
        Physics::ActorId groundactor = Physics::CreateActorInstance(ground, Math::matrix44::identity(),false);
        this->ballResource = Resources::CreateResource("phy:test/tower.np", "Viewer", nullptr, nullptr, true);
        //this->ballResource = Resources::CreateResource("phy:test/sphere.np", "Viewer", nullptr, nullptr, true);

        //Physics::ActorContext::CreatePlane(Math::plane(Math::point(0.0, 0.0, 0.0), Math::vector(0.0, 1.0, 0.0)), dummyMaterial);        
        // register visibility system
        ObserverContext::CreateBruteforceSystem({});

        ObservableContext::RegisterEntity(this->entity);
        ObservableContext::Setup(this->entity, VisibilityEntityType::Model);
		ObservableContext::RegisterEntity(this->ground);
		ObservableContext::Setup(this->ground, VisibilityEntityType::Model);
        ObserverContext::RegisterEntity(this->cam);
        ObserverContext::Setup(this->cam, VisibilityEntityType::Camera);

		Characters::CharacterContext::RegisterEntity(this->entity);
		Characters::CharacterContext::Setup(this->entity, "ske:Units/Unit_Archer.nsk3", "ani:Units/Unit_Archer.nax3", "Viewer");
		Characters::CharacterContext::PlayClip(this->entity, nullptr, 0, 0, Characters::Append);

		Util::Array<Graphics::GraphicsEntityId> models;
		ModelContext::BeginBulkRegister();
		ObservableContext::BeginBulkRegister();        
        Resources::ResourceId foo = Resources::CreateResource("phy:test/tower.np", "Viewer", nullptr, nullptr, true);
		static const int NumModels = 1;
		for (IndexT i = -NumModels; i < NumModels; i++)
		{
			for (IndexT j = -NumModels; j < NumModels; j++)
			{
				Graphics::GraphicsEntityId ent = Graphics::CreateEntity();
                this->entities.Append(ent);

				// create model and move it to the front
				ModelContext::RegisterEntity(ent);
				ModelContext::Setup(ent, "mdl:Buildings/castle_tower.n3", "NotA");
                Math::matrix44 trans = Math::matrix44::translation(Math::float4(i * 10, 10, -j * 10, 1));
				ModelContext::SetTransform(ent, trans);

				ObservableContext::RegisterEntity(ent);
				ObservableContext::Setup(ent, VisibilityEntityType::Model);
				models.Append(ent);
                                                                
                Physics::ActorId actor = Physics::CreateActorInstance(foo, trans, true);
                this->idMap.Add(actor.id, ent);
                Physics::ActorContext::GetActor(actor).moveCallback = 
                    Util::Delegate<void(Physics::ActorId, Math::matrix44 const&)>::FromMethod<SimpleViewerApplication,&SimpleViewerApplication::UpdateTransform>(this);
			}
		}
		ModelContext::EndBulkRegister();
		ObservableContext::EndBulkRegister();
        this->UpdateCamera();

        

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::Close()
{
    DestroyWindow(this->wnd);
    this->gfxServer->DiscardStage(this->stage);
    this->gfxServer->DiscardView(this->view);

    this->gfxServer->Close();
    this->inputServer->Close();
    this->resMgr->Close();
    this->debugInterface->Close();
    this->Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::Run()
{    
    bool run = true;

    const Ptr<Input::Keyboard>& keyboard = inputServer->GetDefaultKeyboard();
    const Ptr<Input::Mouse>& mouse = inputServer->GetDefaultMouse();
        
    while (run && !inputServer->IsQuitRequested())
    {                             
        const Timing::Time delta = this->gfxServer->GetFrameTime();
        Physics::Update(delta);
        this->fpsGraph.AddValue(1.0f / delta);
        this->inputServer->BeginFrame();
        this->inputServer->OnFrame();

        this->resMgr->Update(this->frameIndex);

		// animate the spotlights
		IndexT i;
		for (i = 0; i < 3; i++)
		{
			Math::matrix44 spotLightTransform;
			Math::scalar scaleFactor = i * 1.5f + 30;
			spotLightTransform.scale(Math::point(scaleFactor, scaleFactor, scaleFactor + 10));
			spotLightTransform = Math::matrix44::multiply(spotLightTransform, Math::matrix44::rotationyawpitchroll(this->gfxServer->GetTime() * 2 * (i + 1) / 3, Math::n_deg2rad(-55), 0));
			spotLightTransform.set_position(Lighting::LightContext::GetTransform(this->spotLights[i]).get_position());
			Lighting::LightContext::SetTransform(this->spotLights[i], spotLightTransform);
		}

        this->gfxServer->BeginFrame();
        
        // put game code which doesn't need visibility data or animation here
        this->gfxServer->BeforeViews();
        this->RenderUI();             

        if (this->renderDebug)
        {
            this->gfxServer->RenderDebug(0);
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
                
        if(this->inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::LeftMenu))
            this->UpdateCamera();

        if (this->inputServer->GetDefaultKeyboard()->KeyPressed(Input::Key::Space)) 
        {
            this->Shoot();
        }
        

        frameIndex++;             
        this->inputServer->EndFrame();
    }
}

//------------------------------------------------------------------------------
/**
*/
static const char * 
GraphicsEntityToName(GraphicsEntityId id)
{
    if (ModelContext::IsEntityRegistered(id)) return "Model";
    if (Lighting::LightContext::IsEntityRegistered(id)) return "Light";
    return "Entity";
}

//------------------------------------------------------------------------------
/**
*/
void
SimpleViewerApplication::RenderEntityUI()
{
    ImGui::Begin("Entities", nullptr, 0);
	ImGui::SetWindowSize(ImVec2(240, 400));
    ImGui::BeginChild("##entities", ImVec2(0, 300), true);
    static int selected = 0;
    for (int i = 0 ; i < this->entities.Size();i++)
    {
        Util::String sid;
        sid.Format("%s: %d", GraphicsEntityToName(this->entities[i]), this->entities[i]);
        if (ImGui::Selectable(sid.AsCharPtr(), i == selected))
        {
            selected = i;
        }        
    }
    ImGui::EndChild();
    ImGui::End();
    auto id = this->entities[selected];
    if (ModelContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = ModelContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {            
            ModelContext::SetTransform(id, trans);
        }
    }            
    else if (Lighting::LightContext::IsEntityRegistered(id))
    {
        Im3d::Mat4 trans = Lighting::LightContext::GetTransform(id);
        if (Im3d::Gizmo("GizmoEntity", trans))
        {
            Lighting::LightContext::SetTransform(id, trans);
        }
    }
}
//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::RenderUI()
{
    ImGui::Begin("fps", nullptr, 0);
    this->fpsGraph.Draw();
    ImGui::End();
    ImGui::Begin("Viewer", nullptr, 0);
	ImGui::SetWindowSize(ImVec2(240, 400));
    ImGui::Text("%f fps", 1.0f / this->gfxServer->GetFrameTime());
    if (ImGui::CollapsingHeader("Camera mode", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::RadioButton("Maya", &this->cameraMode, 0))this->ToMaya();
        ImGui::SameLine();
        if (ImGui::RadioButton("Free", &this->cameraMode, 1))this->ToFree();
        ImGui::SameLine();
        if (ImGui::Button("Reset")) this->ResetCamera();
    }
    ImGui::Checkbox("Debug Rendering", &this->renderDebug);
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
    this->RenderEntityUI();
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::UpdateCamera()
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

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::ResetCamera()
{
    this->freeCamUtil.Setup(this->defaultViewPoint, Math::float4::normalize(this->defaultViewPoint));
    this->freeCamUtil.Update();
    this->mayaCameraUtil.Setup(Math::point(0.0f, 0.0f, 0.0f), this->defaultViewPoint, Math::vector(0.0f, 1.0f, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::ToMaya()
{
    this->mayaCameraUtil.Setup(this->mayaCameraUtil.GetCenterOfInterest(), this->freeCamUtil.GetTransform().get_position(), Math::vector(0, 1, 0));
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::ToFree()
{
    Math::float4 pos = this->mayaCameraUtil.GetCameraTransform().get_position();
    this->freeCamUtil.Setup(pos, Math::float4::normalize(pos - this->mayaCameraUtil.GetCenterOfInterest()));
}

//------------------------------------------------------------------------------
/**
*/
void 
SimpleViewerApplication::Browse()
{
    this->folders = IO::IoServer::Instance()->ListDirectories("mdl:", "*");    
    this->files = IO::IoServer::Instance()->ListFiles("mdl:" + this->folders[this->selectedFolder], "*");
}

//------------------------------------------------------------------------------
/**
*/
void
SimpleViewerApplication::Shoot()
{
    static Timing::Time last = 0.0;
    Timing::Time now = this->gfxServer->GetTime();
    if (now - last > 0.05) 
    {
        last = now;

        Math::matrix44 trans = Math::matrix44::inverse(CameraContext::GetTransform(this->cam));



        Graphics::GraphicsEntityId ent = Graphics::CreateEntity();

        ModelContext::RegisterEntity(ent);
        ModelContext::Setup(ent, "mdl:Buildings/castle_tower.n3", "NotA");
        //ModelContext::Setup(ent, "mdl:system/sphere.n3", "NotA");
        ModelContext::SetTransform(ent, trans);

        ObservableContext::RegisterEntity(ent);
        ObservableContext::Setup(ent, VisibilityEntityType::Model);
        this->entities.Append(ent);

        Physics::ActorId actor = Physics::CreateActorInstance(this->ballResource, trans, true);
        this->idMap.Add(actor.id, ent);
        
        Physics::ActorContext::GetActor(actor).moveCallback = Util::Delegate<void(Physics::ActorId, Math::matrix44 const&)>::FromMethod<SimpleViewerApplication, &SimpleViewerApplication::UpdateTransform>(this);
        
        Physics::ActorContext::SetLinearVelocity(actor, trans.get_zaxis() * -50.0f);
        Physics::ActorContext::SetAngularVelocity(actor, Math::vector(Math::n_rand(-10.0f, 10.0f)));
    }

}

//------------------------------------------------------------------------------
/**
*/
void
SimpleViewerApplication::UpdateTransform(Physics::ActorId id, Math::matrix44 const & trans)
{
    ModelContext::SetTransform(this->idMap[id.id], trans);
}
}
