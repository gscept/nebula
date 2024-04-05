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
#include "resources/resourceserver.h"

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
    switch (this->assetType)
    {
    case PreviewAssetType::Material:
        break;
    case PreviewAssetType::Mesh:
        break;
    case PreviewAssetType::Skeleton:
        break;
    case PreviewAssetType::Model:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Previewer::Run()
{
    //this->viewport.Render();
}

//------------------------------------------------------------------------------
/**
*/
void
Previewer::Preview(const Resources::ResourceName& asset)
{
    Resources::CreateResource(
        asset,
        "editor",
        [this](Resources::ResourceId id)
        {
            this->asset.id = id.resourceId;
        }
    );
}

} // namespace Presentation
