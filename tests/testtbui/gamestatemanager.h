#pragma once
//------------------------------------------------------------------------------
/**
    @class  Tests::GameStateManager

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "renderutil/mouserayutil.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "input/inputserver.h"
#include "io/ioserver.h"
#include "graphics/cameracontext.h"
#include "models/nodes/primitivenode.h"
#include "models/modelcontext.h"
#include "imgui.h"
#include "dynui/im3d/im3d.h"
#include "dynui/im3d/im3dcontext.h"
#include "timing/timer.h"
#include "tbui/tbuiview.h"

#include "tb_widgets.h"
#include "tb_widgets_common.h"
#include "tb_widgets_reader.h"
#include "tb_widgets_listener.h"
#include "tb_message_window.h"
#include "tb_msg.h"
#include "tb_scroller.h"
#include "ListWindow.h"
#include "ResourceEditWindow.h"

namespace Tests
{
using namespace tb;

class MainWindow : public DemoWindow, public TBMessageHandler
{
public:
    MainWindow(TBWidget* root);
    virtual bool OnEvent(const TBWidgetEvent& ev);

    // Implement TBMessageHandler
    virtual void OnMessageReceived(TBMessage* msg);
};

class ImageWindow : public DemoWindow
{
public:
    ImageWindow(TBWidget* root);
    virtual bool OnEvent(const TBWidgetEvent& ev);
};

class PageWindow : public DemoWindow, public TBScrollerSnapListener
{
public:
    PageWindow(TBWidget* root);
    virtual bool OnEvent(const TBWidgetEvent& ev);
    virtual void OnScrollSnap(TBWidget* target_widget, int& target_x, int& target_y);
};

class AnimationsWindow : public DemoWindow
{
public:
    AnimationsWindow(TBWidget* root);
    void Animate();
    virtual bool OnEvent(const TBWidgetEvent& ev);
};

class LayoutWindow : public DemoWindow
{
public:
    LayoutWindow(TBWidget* root, const char* filename);
    virtual bool OnEvent(const TBWidgetEvent& ev);
};

class TabContainerWindow : public DemoWindow
{
public:
    TabContainerWindow(TBWidget* root);
    virtual bool OnEvent(const TBWidgetEvent& ev);
};

class ConnectionWindow : public DemoWindow
{
public:
    ConnectionWindow(TBWidget* root);
    virtual bool OnEvent(const TBWidgetEvent& ev);
};

class ScrollContainerWindow : public DemoWindow, public TBMessageHandler
{
public:
    ScrollContainerWindow(TBWidget* root);
    virtual bool OnEvent(const TBWidgetEvent& ev);

    // Implement TBMessageHandler
    virtual void OnMessageReceived(TBMessage* msg);
};

class GameStateManager : public Game::Manager
{
    __DeclareClass(GameStateManager)
    __DeclareSingleton(GameStateManager)
public:
    GameStateManager();
    virtual ~GameStateManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
    void OnFrame() override;

    private:
    TBUI::TBUIView* view = nullptr;
};

} // namespace Demo
