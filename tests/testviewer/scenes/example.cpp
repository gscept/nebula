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
Graphics::GraphicsEntityId gid;
float v = 0.0f;

//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    entity = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    Models::ModelContext::Setup(entity, "mdl:system/placeholder.n3", "ExampleScene", []()
    {
        Visibility::ObservableContext::Setup(entity, Visibility::VisibilityEntityType::Model);
    });
    Models::ModelContext::SetTransform(entity, Math::translation(Math::vec3(0, 0, 0)));

    otherEntity = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(otherEntity);
    Models::ModelContext::Setup(otherEntity, "mdl:system/placeholder.n3", "ExampleScene", []()
    {
        Visibility::ObservableContext::Setup(otherEntity, Visibility::VisibilityEntityType::Model);
    });
    Models::ModelContext::SetTransform(otherEntity, Math::translation(Math::vec3(2, 0, 0)));

    v = 0.0f;
};

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    Visibility::ObservableContext::DeregisterEntity(entity);
    Models::ModelContext::DeregisterEntity(entity);
    Graphics::DestroyEntity(entity);
    Visibility::ObservableContext::DeregisterEntity(otherEntity);
    Models::ModelContext::DeregisterEntity(otherEntity);
    Graphics::DestroyEntity(otherEntity);

    if (Models::ModelContext::IsEntityRegistered(gid))
    {
        if (Visibility::ObservableContext::IsEntityRegistered(gid))
            Visibility::ObservableContext::DeregisterEntity(gid);

        Models::ModelContext::DeregisterEntity(gid);
        Graphics::DestroyEntity(gid);
    }
};

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    Models::ModelContext::SetTransform(entity, Math::translation(Math::vec3(0, 0, Math::sin(v))));
    v += 0.01f;

    // create a delete entities each 30 frames
    static int frameIndex = 0;
    
    if (frameIndex % 60 == 0)
    {
        gid = Graphics::CreateEntity();
        Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(gid);
        Models::ModelContext::Setup(gid, "mdl:system/placeholder.n3", "ExampleScene", []()
        {
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
            Models::ModelContext::SetTransform(gid, Math::translation(Math::vec3(5, 0, 0)));
        });
    }
    else if (frameIndex % 30 == 0)
    {
        if (Models::ModelContext::IsEntityRegistered(gid))
        {
            Visibility::ObservableContext::DeregisterEntity(gid);
            Models::ModelContext::DeregisterEntity(gid);
            Graphics::DestroyEntity(gid);
        }
    }

    frameIndex++;
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