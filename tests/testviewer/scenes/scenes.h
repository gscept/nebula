#pragma once
#include <functional>

struct Scene
{
    const char* name = nullptr;
    std::function<void()> Open;
    std::function<void()> Close;
    std::function<void()> Run;
    std::function<void()> RenderUI;
};

// List all extern scenes here
extern Scene ExampleScene;
extern Scene ClusteredScene;
extern Scene SSRScene;
extern Scene BenchmarkScene;
extern Scene SponzaScene;
extern Scene BistroScene;
extern Scene TerrainScene;
extern Scene PhysicsScene;

// Don't forget to add them to the list of scenes!
static Scene* scenes[] = 
{
    &ExampleScene,
    &ClusteredScene,
    &BenchmarkScene,
    &SSRScene,
    &SponzaScene,
    &BistroScene,
    &TerrainScene,
    &PhysicsScene
};

enum
{
    ExampleSceneId,
    ClusteredSceneId,
    BenchmarkSceneId,
    SSRSceneId,
    SponzaSceneId,
    BistroSceneId,
    TerrainSceneId,
    PhysicsSceneId
};

// Which is the currently active scene?
// This is used to determine which scenes callbacks to call and is normally changed via imgui
extern int currentScene;
