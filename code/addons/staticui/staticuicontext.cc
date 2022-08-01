//------------------------------------------------------------------------------
//  @file staticuicontext.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "staticuicontext.h"
#include "Ultralight/Ultralight.h"
#include "AppCore/AppCore.h"
#include "Ultralight/JavaScript.h"
#include "frame/framecode.h"
#include "frame/framesubgraph.h"
#include "graphics/graphicsserver.h"
#include "jobs2/jobs2.h"
#include "profiling/profiling.h"
namespace StaticUI
{

struct Logger : ultralight::Logger
{
    void LogMessage(ultralight::LogLevel log_level, const ultralight::String16& message) override
    {
        n_printf("Ultralight [Severity %d]: %s\n", log_level, ultralight::String(message).utf8().data());
    }
};

struct
{
    ultralight::RefPtr<ultralight::View> view;
    ultralight::RefPtr<ultralight::Renderer> Renderer;

    Logger logger;
    ultralight::ViewListener viewListener;
    ultralight::LoadListener loadListener;
    UltralightRenderer* Backend;
    uint counter;
} state;

__ImplementPluginContext(StaticUIContext)
//------------------------------------------------------------------------------
/**
*/
StaticUIContext::StaticUIContext()
{

}

//------------------------------------------------------------------------------
/**
*/
StaticUIContext::~StaticUIContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
StaticUIContext::Create()
{
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    ultralight::Config config;
    //config.resource_path = IO::URI("ui:").AsString().AsCharPtr();
    config.face_winding = ultralight::FaceWinding::kFaceWinding_CounterClockwise;
    config.resource_path = "./resources/";
    config.device_scale = 1.0f;
    config.use_gpu_renderer = true;

    state.Backend = new UltralightRenderer;

    ultralight::Platform::instance().set_font_loader(ultralight::GetPlatformFontLoader());
    ultralight::Platform::instance().set_file_system(ultralight::GetPlatformFileSystem("."));
    ultralight::Platform::instance().set_logger(&state.logger);
    ultralight::Platform::instance().set_gpu_driver(state.Backend);
    ultralight::Platform::instance().set_config(config);
    
    state.Renderer = ultralight::Renderer::Create();
    state.view = state.Renderer->CreateView(1280, 1024, false, nullptr);
    state.view->set_load_listener(&state.loadListener);
    state.view->set_view_listener(&state.viewListener);
    //state.view->LoadHTML("<body><h1 style='font-size: 40pt'>Hello World</h1></body>");
    //state.view->LoadURL("file:///H:/nota/fips-deploy/nota/vulkan-win64-vstudio-release/test.html");
    //state.view->LoadURL("https://www.google.co.uk");
    state.view->LoadURL("https://www.nk.se");
    state.view->Focus();

    // Create callback for JS messages
    auto js = state.view->LockJSContext();
    JSStringRef nativeBoxFuncName = JSStringCreateWithUTF8CString("NativeMessageBox");
    JSObjectRef nativeBoxFunc = JSObjectMakeFunctionWithCallback(js.get().ctx(), nativeBoxFuncName, [](JSContextRef ctx, JSObjectRef func, JSObjectRef object, size_t argCount, const JSValueRef args[], JSValueRef* exception) -> JSValueRef
    {
        return JSValueMakeNull(ctx);
    });
    JSObjectRef global = JSContextGetGlobalObject(js.get().ctx());
    JSObjectSetProperty(js.get().ctx(), global, nativeBoxFuncName, nativeBoxFunc, 0, nullptr);
    JSStringRelease(nativeBoxFuncName);

    Frame::FrameCode* drawOp = new Frame::FrameCode;
    drawOp->domain = CoreGraphics::BarrierDomain::Global;
    drawOp->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        state.Backend->Render(cmdBuf, bufferIndex);
    };
    Frame::AddSubgraph("StaticUI", { drawOp });

    Frame::FrameCode* copyOp = new Frame::FrameCode;
    copyOp->domain = CoreGraphics::BarrierDomain::Pass;
    copyOp->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        state.Backend->DrawToBackbuffer(cmdBuf, bufferIndex);
    };
    Frame::AddSubgraph("StaticUI To Backbuffer", { copyOp });

}

//------------------------------------------------------------------------------
/**
*/
void
StaticUIContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void
StaticUIContext::Render()
{
    state.Renderer->Update();
    state.Renderer->Render();

    state.Backend->PreDraw(state.view->render_target());
}

//------------------------------------------------------------------------------
/**
*/
void
StaticUIContext::RegisterFunction(const char* name, std::function<void()> func)
{
    // Create callback for JS messages
    auto js = state.view->LockJSContext();
    JSStringRef funcName = JSStringCreateWithUTF8CString(name);
    JSObjectRef jsFunc = JSObjectMakeFunctionWithCallback(js.get().ctx(), funcName, [](JSContextRef ctx, JSObjectRef func, JSObjectRef object, size_t argCount, const JSValueRef args[], JSValueRef* exception) -> JSValueRef
    {
        return JSValueMakeNull(ctx);
    });
    //JSObjectSetProperty(js.get().ctx(), jsFunc, JSStringCreateWithUTF8CString("this"), 
    //JSObjectRef global = JSContextGetGlobalObject(js.get().ctx());
    //JSObjectSetProperty(js.get().ctx(), global, funcName, jsFunc, 0, nullptr);
    JSStringRelease(funcName);
}

} // namespace StaticUI
