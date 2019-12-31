//------------------------------------------------------------------------------
/*
    This is an example of a scene file
    
    The actual scene object is defined at the bottom of the file. This object
    should be extern in the scenes.h file and added to the list of scenes.

    Functions and data is defined within a namespace, so that we don't pollute
    the global namespace too much.

*/
// (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenes.h"

namespace ExampleSceneData
{

// Global variables, within namespace
Graphics::GraphicsEntityId entity;
Graphics::GraphicsEntityId otherEntity;
float v = 0.0f;

//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    entity = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    Models::ModelContext::Setup(entity, "mdl:system/placeholder.n3", "ExampleScene");
    Models::ModelContext::SetTransform(entity, Math::matrix44::translation(Math::float4(0, 0, 0, 1)));
    Visibility::ObservableContext::Setup(entity, Visibility::VisibilityEntityType::Model);
    
    otherEntity = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(otherEntity);
    Models::ModelContext::Setup(otherEntity, "mdl:system/placeholder.n3", "ExampleScene");
    Models::ModelContext::SetTransform(otherEntity, Math::matrix44::translation(Math::float4(2, 0, 0, 1)));
    Visibility::ObservableContext::Setup(otherEntity, Visibility::VisibilityEntityType::Model);

    v = 0.0f;
};

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    Graphics::DestroyEntity(entity);
    Graphics::DestroyEntity(otherEntity);
};

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    Models::ModelContext::SetTransform(entity, Math::matrix44::translation(Math::float4(0, 0, Math::n_sin(v), 1)));
    v += 0.01f;
};

//------------------------------------------------------------------------------
/**
    ImGui code can be placed here.
*/
void RenderUI()
{
    // empty
};

} // namespace ExampleSceneData

// ---------------------------------------------------------

Scene ExampleScene =
{
    "ExampleScene",
    ExampleSceneData::OpenScene,
    ExampleSceneData::CloseScene,
    ExampleSceneData::StepFrame,
    ExampleSceneData::RenderUI
};