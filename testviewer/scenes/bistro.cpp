//------------------------------------------------------------------------------
// (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenes.h"

namespace BistroSceneData
{

// Global variables, within namespace
Graphics::GraphicsEntityId entity;


//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    entity = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    Models::ModelContext::Setup(entity, "mdl:bistro/Bistro_Exterior.n3", "BistroScene");
    
    Math::mat4 t = Math::translation(Math::vec3(0, 0, 0));
    Math::mat4 r = Math::rotationx(Math::n_deg2rad(0.0f));
    Math::mat4 s = Math::scaling(100);
    
    Math::mat4 trs = s * r;
    trs = trs * t;
    
    Models::ModelContext::SetTransform(entity, trs);
    Visibility::ObservableContext::Setup(entity, Visibility::VisibilityEntityType::Model);
};

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    Graphics::DeregisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    Graphics::DestroyEntity(entity);

};

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    // empty
};

//------------------------------------------------------------------------------
/**
    ImGui code can be placed here.
*/
void RenderUI()
{
    // empty
};

} // namespace BistroSceneData

// ---------------------------------------------------------

Scene BistroScene =
{
    "BistroScene",
    BistroSceneData::OpenScene,
    BistroSceneData::CloseScene,
    BistroSceneData::StepFrame,
    BistroSceneData::RenderUI
};