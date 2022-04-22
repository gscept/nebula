//------------------------------------------------------------------------------
//  previewer.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "previewer.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "graphics/graphicsserver.h"

using namespace Editor;

namespace Presentation
{
__ImplementClass(Presentation::Previewer, 'PrvW', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Previewer::Previewer()
{
    Ptr<Graphics::GraphicsServer> gfxServer = Graphics::GraphicsServer::Instance();

    // FIXME: Can't setup framescript twice at the moment. Fix this when the issue has been resolved
    return;
    this->graphicsView = gfxServer->CreateView("editor_preview", "frame:vkdefault.json");
    this->graphicsStage = gfxServer->CreateStage("editor_preview", true);
    this->graphicsView->SetStage(this->graphicsStage);

    this->viewport.Init(this->graphicsView);
    this->viewport.SetStage(this->graphicsStage);
    this->viewport.SetFrameBuffer("ColorBufferNoGUI");
}

//------------------------------------------------------------------------------
/**
*/
Previewer::~Previewer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Previewer::Update()
{
    //this->viewport.Update();
}

//------------------------------------------------------------------------------
/**
*/
void
Previewer::Run()
{
    //this->viewport.Render();
}

} // namespace Presentation
