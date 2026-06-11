//------------------------------------------------------------------------------
//  main.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "application/stdneb.h"
#include "system/appentry.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "appgame/gameapplication.h"
#include "tbuifeatureunit.h"
#include "gamestatemanager.h"
#include "profiling/profiling.h"
#include "editorfeature/editorfeatureunit.h"
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/options/levelsettings.h"

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

        IO::URI tablePath = "proj:work/data/tables/base_level.json"_uri;
        CompileFlatbuffer(App::LevelSettings, tablePath, "tbl:app");
        
        this->EnableRuntimeModule("editorfeaturemodule", true);

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
    }

    Ptr<GraphicsFeature::GraphicsFeatureUnit> graphicsFeature;
    Ptr<Tests::TBUIFeatureUnit> demoFeatureUnit;
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
