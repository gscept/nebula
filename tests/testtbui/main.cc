//------------------------------------------------------------------------------
//  main.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define NEBULA_EDITOR_ENABLED

#include "application/stdneb.h"
#include "system/appentry.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "appgame/gameapplication.h"
#include "tbuifeatureunit.h"
#include "gamestatemanager.h"
#include "profiling/profiling.h"

#ifdef NEBULA_EDITOR_ENABLED
#include "editorfeature/editorfeatureunit.h"
#endif

#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/graphicsfeature/terrainschema.h"

ImplementNebulaApplication();

using namespace Core;


class NebulaDemoApplication : public App::GameApplication
{
private:
    /// setup game features
    void SetupGameFeatures()
    {
#if NEBULA_ENABLE_PROFILING
        Profiling::ProfilingRegisterThread();
#endif
        this->graphicsFeature = GraphicsFeature::GraphicsFeatureUnit::Create();
        this->graphicsFeature->SetCmdLineArgs(this->GetCmdLineArgs());
        this->gameServer->AttachGameFeature(this->graphicsFeature);

        this->demoFeatureUnit = Tests::TBUIFeatureUnit::Create();
        this->gameServer->AttachGameFeature(this->demoFeatureUnit);

#ifdef NEBULA_EDITOR_ENABLED
        this->editorFeatureUnit = EditorFeature::EditorFeatureUnit::Create();
        this->gameServer->AttachGameFeature(this->editorFeatureUnit);
#endif

        Flat::FlatbufferInterface::LoadSchema("data:flatbuffer/graphicsfeature/terrainschema.bfbs"_uri);
        IO::URI tablePath = "proj:work/data/tables/terrain.json"_uri;
        CompileFlatbuffer(GraphicsFeature::TerrainSetup, tablePath, "data:tables/graphicsfeature");
    }
    /// cleanup game features
    void CleanupGameFeatures()
    {
        this->gameServer->RemoveGameFeature(this->graphicsFeature);
        this->gameServer->RemoveGameFeature(this->demoFeatureUnit);
        this->graphicsFeature->Release();
        this->graphicsFeature = nullptr;
        this->demoFeatureUnit->Release();
        this->demoFeatureUnit = nullptr;

#ifdef NEBULA_EDITOR_ENABLED
        this->gameServer->RemoveGameFeature(this->editorFeatureUnit);
        this->editorFeatureUnit->Release();
        this->editorFeatureUnit = nullptr;
#endif
    }

    Ptr<GraphicsFeature::GraphicsFeatureUnit> graphicsFeature;
    Ptr<Tests::TBUIFeatureUnit> demoFeatureUnit;

#ifdef NEBULA_EDITOR_ENABLED
    Ptr<EditorFeature::EditorFeatureUnit> editorFeatureUnit;
#endif
};

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    NebulaDemoApplication gameApp;
    gameApp.SetCompanyName("Gscept");
    gameApp.SetAppTitle("Turbobadger demo");
    gameApp.SetCmdLineArgs(args);

    if (!gameApp.Open())
    {
        return;
    }

    gameApp.Run();
    gameApp.Close();
    Core::SysFunc::Exit(0);
}
