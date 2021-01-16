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
#include "resources/resourceserver.h"
#include "models/modelcontext.h"
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "debug/debuginterface.h"

#if NEBULA_ENABLE_PROFILING
#include "profiling/profiling.h"
#endif

// comment out to use local content
//#define USE_GITHUB_DEMO 1


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
    void UpdateCamera();
    void ResetCamera();
    void ToMaya();
    void ToFree();
    void Browse();

    Ptr<Graphics::GraphicsServer> gfxServer;
    Ptr<Resources::ResourceServer> resMgr;
    Ptr<Input::InputServer> inputServer;
    Ptr<IO::IoServer> ioServer;

    CoreGraphics::WindowId wnd;
    Ptr<Graphics::View> view;
    Ptr<Graphics::Stage> stage;

    Graphics::GraphicsEntityId cam;
    
    Graphics::GraphicsEntityId globalLight;
    IndexT frameIndex = -1;

    Util::Array<Util::String> folders;
    int selectedFolder = 0;
    Util::Array<Util::String> files;
    int selectedFile = 0;

    Math::transform44 transform;
    float prevAverageFrameTime = 0.0f;
    float averageFrameTime = 0.0f;
    float currentFrameTime = 0.0f;

    bool showCameraWindow = true;
    bool showFrameProfiler = true;
    bool showSceneUI = true;
    bool showGrid = false;

    bool renderDebug = false;
    int cameraMode = 0;
    float zoomIn = 0.0f;
    float zoomOut = 0.0f;
    Math::vec2 panning{ 0.0f,0.0f };
    Math::vec2 orbiting{ 0.0f,0.0f };
    RenderUtil::MayaCameraUtil mayaCameraUtil;
    RenderUtil::FreeCameraUtil freeCamUtil;        
    Math::vec3 defaultViewPoint{ 8.0f, 8.0f, 0.0f };
    Util::Array<float> frametimeHistory;
    
#if __NEBULA_HTTP__
    Ptr<Debug::DebugInterface> debugInterface;
    ushort defaultTcpPort;
#endif

#ifdef NEBULA_ENABLE_PROFILING
    Util::Array<CoreGraphics::FrameProfilingMarker> frameProfilingMarkers;
    Util::Array<Profiling::ProfilingContext> profilingContexts;
    bool pauseProfiling;
    bool profileFixedFps;
    int fixedFps;
#endif

};

}

