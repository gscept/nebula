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
    
    Math::matrix44 t = Math::matrix44::translation(Math::float4(0, 0, 0, 1));
    Math::matrix44 r = Math::matrix44::rotationx(Math::n_deg2rad(0.0f));
    Math::matrix44 s = Math::matrix44::scaling(Math::float4(100, 100, 100, 1));
    
    Math::matrix44 trs = Math::matrix44::multiply(s, r);
    trs = Math::matrix44::multiply(trs, t);
    
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