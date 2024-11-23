//------------------------------------------------------------------------------
//  editorfeatureunit.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorfeatureunit.h"
#include "editor/editor.h"
#include "editor/ui/uimanager.h"
#include "editor/components/editorcomponents.h"
#include "editor/bindings/editorbindings.h"

#include "graphicsfeature/components/model.h"
#include "graphicsfeature/components/decal.h"
#include "graphicsfeature/components/lighting.h"
#include "dynui/im3d/im3dcontext.h"

// TEMP: Move this to editor game manager
#include "game/world.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "lighting/lightcontext.h"
#include "models/modelcontext.h"
#include "decals/decalcontext.h"

namespace EditorFeature
{

__ImplementClass(EditorFeature::EditorFeatureUnit, 'edfu', Game::FeatureUnit);
__ImplementSingleton(EditorFeature::EditorFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
EditorFeatureUnit::EditorFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
EditorFeatureUnit::~EditorFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnAttach()
{
    this->RegisterComponentType<Editor::EditorEntity>();
    Scripting::RegisterEditorBinds();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();
    if (this->args.GetBoolFlag("-editor"))
    {
        Editor::Create();

        this->AttachManager(Editor::UIManager::Create());

        Im3d::Im3dContext::SetGridStatus(true);
        Im3d::Im3dContext::SetGridSize(1.0f, 25);
        Im3d::Im3dContext::SetGridColor(Math::vec4(0.2f, 0.2f, 0.2f, 0.8f));

        Editor::Start();

        // TODO: move this to a game manager that is created by the editor

        Game::World* world = Game::GetWorld(WORLD_DEFAULT);
        Game::ProcessorBuilder(world, "EditorGameManager.UpdateModelTransforms"_atm)
            .On("OnEndFrame")
            .OnlyModified()
            .RunInEditor()
            .Func(
                [](Game::World* world, Game::Position const& pos, Game::Orientation const& orient, Game::Scale const& scale, GraphicsFeature::Model const& model)
                {
                    Math::mat4 worldTransform = Math::trs(pos, orient, scale);
                    Models::ModelContext::SetTransform(model.graphicsEntityId, worldTransform);
                }
            )
            .Build();

        Game::ProcessorBuilder(world, "EditorGameManager.UpdatePointLightPositions"_atm)
            .On("OnEndFrame")
            .OnlyModified()
            .RunInEditor()
            .Func(
                [](Game::World* world,
                   Game::Position const& pos,
                   GraphicsFeature::PointLight const& light)
                {
                    Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                }
            )
            .Build();

        Game::ProcessorBuilder(world, "EditorGameManager.UpdateSpotLightTransform"_atm)
            .On("OnEndFrame")
            .OnlyModified()
            .RunInEditor()
            .Func(
                [](Game::World* world,
                   Game::Position const& pos,
                   Game::Orientation const& rot,
                   GraphicsFeature::SpotLight const& light)
                {
                    Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                    Lighting::LightContext::SetRotation(light.graphicsEntityId, rot);
                }
            )
            .Build();

        Game::ProcessorBuilder(world, "EditorGameManager.UpdateAreaLightTransform"_atm)
            .On("OnEndFrame")
            .OnlyModified()
            .RunInEditor()
            .Func(
                [](Game::World* world,
                   Game::Position const& pos,
                   Game::Orientation const& rot,
                   Game::Scale const& scale,
                   GraphicsFeature::AreaLight const& light)
                {
                    Lighting::LightContext::SetPosition(light.graphicsEntityId, pos);
                    Lighting::LightContext::SetRotation(light.graphicsEntityId, rot);
                    Lighting::LightContext::SetScale(light.graphicsEntityId, scale);
                }
            )
            .Build();

        Game::ProcessorBuilder(world, "EditorGameManager.UpdateDecalTransform"_atm)
            .On("OnEndFrame")
            .OnlyModified()
            .RunInEditor()
            .Func(
                [](Game::World* world,
                   Game::Position const& pos,
                   Game::Orientation const& rot,
                   Game::Scale const& scale,
                   GraphicsFeature::Decal const& decal)
                {
                    Math::mat4 transform = Math::trs(pos, rot, scale);
                    Decals::DecalContext::SetTransform(decal.graphicsEntityId, transform);
                }
            )
            .Build();

        //if (!Editor::ConnectToBackend(...))
        //    Editor::SpawnLocalBackend();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnDeactivate()
{
    FeatureUnit::OnDeactivate();
    if (this->args.GetBoolFlag("-editor"))
    {
        Editor::Destroy();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnRenderDebug()
{
    FeatureUnit::OnRenderDebug();
}

//------------------------------------------------------------------------------
/**
*/
void
EditorFeatureUnit::OnFrame()
{
    FeatureUnit::OnFrame();
}

} // namespace EditorFeature
