#pragma once
//------------------------------------------------------------------------------
/**
    @class Tests::SimpleViewerApplication
    
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "app/application.h"
#include "renderutil/mayacamerautil.h"
#include "renderutil/freecamerautil.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "graphics/cameracontext.h"
#include "resources/resourcemanager.h"
#include "models/modelcontext.h"
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "dynui/graph/imguigraph.h"
#include "debug/debuginterface.h"
#include "physicsinterface.h"


namespace Tests
{

class SimpleViewerApplication : public App::Application
{
public:
    /// Constructor
    SimpleViewerApplication();
    /// Destructor
    ~SimpleViewerApplication();

    /// Open
    bool Open();
    /// Close
    void Close();
    /// Run
    void Run();

protected:
    void RenderUI();
    void RenderEntityUI();
    void RenderObjectsUI();
    void RenderObjectUI(IndexT id);
    void UpdateCamera();
    void ResetCamera();
    void ToMaya();
    void ToFree();
    void Browse();
    void Shoot();
    void Spawn(const Math::matrix44& trans, Math::vector linvel, Math::vector angvel);
    void UpdateTransform(Physics::ActorId id, Math::matrix44 const& trans);
    

    void SpawnRandom(int amount);
    void DeleteRandom(int amount);


    Ptr<Graphics::GraphicsServer> gfxServer;
    Ptr<Resources::ResourceManager> resMgr;
    Ptr<Input::InputServer> inputServer;
    Ptr<IO::IoServer> ioServer;
    Ptr<Debug::DebugInterface> debugInterface;
    IndexT physicsScene;

    CoreGraphics::WindowId wnd;
    Ptr<Graphics::View> view;
    Ptr<Graphics::Stage> stage;

    Graphics::GraphicsEntityId cam;
    Graphics::GraphicsEntityId entity;

	Graphics::GraphicsEntityId ground;
	Graphics::GraphicsEntityId globalLight;
	Graphics::GraphicsEntityId pointLights[3];
	Graphics::GraphicsEntityId spotLights[3];
    IndexT frameIndex = -1;

    Resources::ResourceId ballResource;
    Physics::ActorId groundActor;
    Resources::ResourceId groundResource;

    Util::Array<Util::String> folders;
    int selectedFolder = 0;
    Util::Array<Util::String> files;
    int selectedFile = 0;

    Math::transform44 transform;

    bool renderDebug = true;
    int cameraMode = 0;
    float zoomIn = 0.0f;
    float zoomOut = 0.0f;
    Math::float2 panning{ 0.0f,0.0f };
    Math::float2 orbiting{ 0.0f,0.0f };    
    RenderUtil::MayaCameraUtil mayaCameraUtil;
    RenderUtil::FreeCameraUtil freeCamUtil;        
    Math::point defaultViewPoint;
    Util::Array<Graphics::GraphicsEntityId> entities;    
    Util::Array<Util::String> entityNames;
    Dynui::Graph fpsGraph;

    struct TestObject
    {
        Graphics::GraphicsEntityId model;
        Physics::ActorId actor;
    };



    Util::Array<TestObject> objects;

};

}