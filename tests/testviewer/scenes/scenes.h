#pragma once
#include <functional>
#include "renderutil/mayacamerautil.h"
#include "renderutil/freecamerautil.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "graphics/cameracontext.h"
#include "resources/resourceserver.h"
#include "models/modelcontext.h"
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "graphics/graphicsentity.h"
#include "visibility/visibilitycontext.h"
#include "models/streammodelpool.h"
#include "models/modelcontext.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "dynui/imguicontext.h"
#include "lighting/lightcontext.h"
#include "characters/charactercontext.h"
#include "imgui.h"
#include "dynui/im3d/im3dcontext.h"
#include "dynui/im3d/im3d.h"
#include "graphics/environmentcontext.h"
#include "clustering/clustercontext.h"
#include "math/mat4.h"
#include "particles/particlecontext.h"
#include "fog/volumetricfogcontext.h"
#include "decals/decalcontext.h"

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

// Don't forget to add them to the list of scenes!
static Scene* scenes[] = 
{
    &ExampleScene,
    &ClusteredScene,
	&BenchmarkScene,
    &SSRScene,
    &SponzaScene,
    &BistroScene
};

enum
{
	ExampleSceneId,
	ClusteredSceneId,
	BenchmarkSceneId,
	SSRSceneId,
    SponzaSceneId,
    BistroSceneId
};

// Which is the currently active scene?
// This is used to determine which scenes callbacks to call and is normally changed via imgui
static int currentScene = SponzaSceneId;
