//------------------------------------------------------------------------------
// (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenes.h"

namespace SSRSceneData
{

// Global variables, within namespace
Graphics::GraphicsEntityId entity;

//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    // entity = Graphics::CreateEntity();
    // Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    // Models::ModelContext::Setup(entity, "mdl:test/sibenik.n3", "SSRScene");
    // Models::ModelContext::SetTransform(entity, Math::matrix44::translation(Math::float4(0, 10, 0, 1)));
    // Visibility::ObservableContext::Setup(entity, Visibility::VisibilityEntityType::Model);
};

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    Graphics::DestroyEntity(entity);
};

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
};

//------------------------------------------------------------------------------
/**
    ImGui code can be placed here.
*/
void RenderUI()
{
    // empty
};

} // namespace SSRSceneData

// ---------------------------------------------------------

Scene SSRScene =
{
    "SSRScene",
    SSRSceneData::OpenScene,
    SSRSceneData::CloseScene,
    SSRSceneData::StepFrame,
    SSRSceneData::RenderUI
};