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
public:

    enum class PreviewAssetType
    {
        None,
        Material,
        Mesh,
        Skeleton,
        Model
    };
    Previewer();
    ~Previewer();

    void Run();

    // Select material for previewing
    void Preview(const Resources::ResourceName& asset, const PreviewAssetType type);

    PreviewAssetType assetType;
    union
    {
        Materials::MaterialId material;
        CoreGraphics::MeshResourceId mesh;
        Characters::SkeletonResourceId skeleton;
        Models::ModelId model;

        Resources::ResourceUnknownId id = Resources::InvalidResourceUnknownId;
    } asset;
};
__RegisterClass(Previewer)

} // namespace Presentation

