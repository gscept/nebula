#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetsFeatureUnit
    The editor's asset side. It sets up what working with assets needs: the work folders an asset
    is authored in, the URN work root, and the live batcher that exports an asset as it changes. It
    registers the asset panels, which know no asset type: a subsystem registers its types with the
    AssetTypeRegistry. The asset index the panels read is kept up to date here every frame, whether a
    panel is open or not, so it is current when one opens.
    The panels have to be registered before the editor opens, so this is attached before the
    editor's own feature unit, and removed after it.
    @see
    Editor::AssetTypeRegistry
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/featureunit.h"

namespace Editor
{

class AssetDatabase;
class AssetPicker;

class AssetsFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(AssetsFeatureUnit)
public:
    AssetsFeatureUnit();
    ~AssetsFeatureUnit();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;

private:
    /// the asset index every asset view reads
    Ptr<AssetDatabase> database;
    /// the popup assets and folders are picked in
    Ptr<AssetPicker> picker;
};

} // namespace Editor
//------------------------------------------------------------------------------
