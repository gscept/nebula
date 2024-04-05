#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Previewer

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/ui/modules/viewport.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "coregraphics/meshresource.h"
#include "characters/skeletonresource.h"

namespace Presentation
{

class Previewer : public BaseWindow
{
    __DeclareClass(Previewer)

    enum class PreviewAssetType
    {
        None,
        Material,
        Mesh,
        Skeleton,
        Model
    };
public:
    Previewer();
    ~Previewer();

    void Update();
    void Run();

    // Select material for previewing
    void Preview(const Resources::ResourceName& asset);

    PreviewAssetType assetType;
    union
    {
        Materials::MaterialId material;
        CoreGraphics::MeshResourceId mesh;
        Characters::SkeletonResourceId skeleton;
        Models::ModelId model;

        Resources::ResourceUnknownId id;
    } asset;

    Modules::Viewport viewport;
    Ptr<Graphics::View> graphicsView;
    Ptr<Graphics::Stage> graphicsStage;
};
__RegisterClass(Previewer)

} // namespace Presentation

