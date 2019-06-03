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
#include "debug/debuginterface.h"

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
    void UpdateCamera();
    void ResetCamera();
    void ToMaya();
    void ToFree();
    void Browse();

    Ptr<Graphics::GraphicsServer> gfxServer;
    Ptr<Resources::ResourceManager> resMgr;
    Ptr<Input::InputServer> inputServer;
    Ptr<IO::IoServer> ioServer;

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

    Util::Array<Util::String> folders;
    int selectedFolder = 0;
    Util::Array<Util::String> files;
    int selectedFile = 0;

    Math::transform44 transform;
	float prevAverageFrameTime = 0.0f;
	float averageFrameTime = 0.0f;

    bool renderDebug = false;
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

#if __NEBULA_HTTP__
	Ptr<Debug::DebugInterface> debugInterface;

	ushort defaultTcpPort;
#endif
};

}