//------------------------------------------------------------------------------
//  main.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define NEBULA_EDITOR_ENABLED

#include "application/stdneb.h"
#include "system/appentry.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "appgame/gameapplication.h"
#include "tbuifeatureunit.h"
#include "gamestatemanager.h"
#include "profiling/profiling.h"
#include "scripting/python/pythonserver.h"
#include "audiofeature/audiofeatureunit.h"

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

        this->physicsFeature = PhysicsFeature::PhysicsFeatureUnit::Create();
        this->gameServer->AttachGameFeature(this->physicsFeature);

        this->audioFeature = AudioFeature::AudioFeatureUnit::Create();
        this->audioFeature->SetCmdLineArgs(this->GetCmdLineArgs());
        this->gameServer->AttachGameFeature(this->audioFeature);

        this->demoFeatureUnit = Tests::TBUIFeatureUnit::Create();
        this->gameServer->AttachGameFeature(this->demoFeatureUnit);

        this->scriptserver = Scripting::PythonServer::Create();
        this->scriptserver->Open();

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
        this->gameServer->RemoveGameFeature(this->physicsFeature);
        this->gameServer->RemoveGameFeature(this->graphicsFeature);
        this->gameServer->RemoveGameFeature(this->demoFeatureUnit);
        this->gameServer->RemoveGameFeature(this->audioFeature);
        this->audioFeature->Release();
        this->audioFeature = nullptr;
        this->physicsFeature->Release();
        this->physicsFeature = nullptr;
        this->graphicsFeature->Release();
        this->graphicsFeature = nullptr;
        this->demoFeatureUnit->Release();
        this->demoFeatureUnit = nullptr;
        this->scriptserver->Release();
        this->scriptserver = nullptr;

#ifdef NEBULA_EDITOR_ENABLED
        this->gameServer->RemoveGameFeature(this->editorFeatureUnit);
        this->editorFeatureUnit->Release();
        this->editorFeatureUnit = nullptr;
#endif
    }

    Ptr<GraphicsFeature::GraphicsFeatureUnit> graphicsFeature;
    Ptr<PhysicsFeature::PhysicsFeatureUnit> physicsFeature;
    Ptr<AudioFeature::AudioFeatureUnit> audioFeature;
    Ptr<Tests::TBUIFeatureUnit> demoFeatureUnit;
    Ptr<Scripting::ScriptServer> scriptserver;

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
