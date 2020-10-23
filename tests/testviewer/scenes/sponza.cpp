//------------------------------------------------------------------------------
// (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "scenes.h"

namespace SponzaSceneData
{

// Global variables, within namespace
Graphics::GraphicsEntityId entity;
Graphics::GraphicsEntityId light;
bool moveLight = true;

float x = 0;
float y = 0;
float speed = 0.005;
float scale = 2;

//------------------------------------------------------------------------------
/**
    Open scene, load resources
*/
void OpenScene()
{
    entity = Graphics::CreateEntity();
    Graphics::RegisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    Models::ModelContext::Setup(entity, "mdl:sponza/sponza.n3", "SponzaScene", []()
        {
            Visibility::ObservableContext::Setup(entity, Visibility::VisibilityEntityType::Model);
        });
    
    Math::mat4 t = Math::translation(Math::vec3(0, 0, 0));
    Math::mat4 r = Math::rotationx(Math::n_deg2rad(0.0f));
    Math::mat4 s = Math::scaling(Math::vec3(1, 1, 1));
    
    Math::mat4 trs = s * r;
    trs = trs * t;
    
    Models::ModelContext::SetTransform(entity, trs);
    
    light = Graphics::CreateEntity();
    Graphics::RegisterEntity<Lighting::LightContext>(light);
    Lighting::LightContext::SetupPointLight(light, Math::vec3(1, 1, 1), 10.0f, Math::translation(0, 1, 0), 10.0f, false);
}

//------------------------------------------------------------------------------
/**
    Close scene, clean up resources
*/
void CloseScene()
{
    Graphics::DeregisterEntity<Models::ModelContext, Visibility::ObservableContext>(entity);
    Graphics::DestroyEntity(entity);
    Graphics::DeregisterEntity<Lighting::LightContext>(light);
    Graphics::DestroyEntity(light);

}

//------------------------------------------------------------------------------
/**
    Per frame callback
*/
void StepFrame()
{
    if (Input::InputServer::Instance()->GetDefaultKeyboard()->KeyPressed(Input::Key::Space))
    {
        moveLight = !moveLight;
    }
    
    if (moveLight)
    {
        Lighting::LightContext::SetTransform(light, Math::translation(Math::n_sin(x) * scale, 0.5f, Math::n_cos(y) * scale));
        x += speed;
        y += speed;
    }
}

//------------------------------------------------------------------------------
/**
    ImGui code can be placed here.
*/
void RenderUI()
{
    // empty
}

} // namespace SponzaSceneData

// ---------------------------------------------------------

Scene SponzaScene =
{
    "SponzaScene",
    SponzaSceneData::OpenScene,
    SponzaSceneData::CloseScene,
    SponzaSceneData::StepFrame,
    SponzaSceneData::RenderUI
};