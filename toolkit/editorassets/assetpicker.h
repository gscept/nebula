#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetPicker
    The popup an asset or a folder is picked in: an asset browser view of its own over the same
    index, so picking does not move what the browser panel shows. Anything that refers to an asset,
    a resource field or the import's destination, asks this for one.
    The answer is given after the frame has drawn, because what takes it usually changes the scene,
    and the panels must not change it while they draw.
    @see
    Editor::AssetBrowser
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "core/singleton.h"
#include "io/path.h"
#include "util/string.h"
#include <functional>

namespace Editor
{

class AssetBrowser;

class AssetPicker : public Core::RefCounted
{
    __DeclareClass(AssetPicker)
    __DeclareInterfaceSingleton(AssetPicker)
public:
    using PickCallback = std::function<void(const IO::Path& picked)>;

    AssetPicker();
    virtual ~AssetPicker();

    /// ask for an asset, starting in the folder of the one given. With a type only assets of it are
    /// shown. The callback gets the asset's URN
    void PickFile(const IO::Path& start, const Util::String& type, const PickCallback& onPick);
    /// ask for a folder, starting in the one given
    void PickFolder(const IO::Path& start, const PickCallback& onPick);
    /// draw the popup. The editor calls this every frame, as one of its modals
    void Draw();

private:
    /// give the picked path to the one who asked
    void RunPicked();
    /// drop the request
    void Cancel();

    Ptr<AssetBrowser> browser;
    PickCallback onPick;
    IO::Path picked;
    bool openRequested;
    bool pickingFolder;
};

} // namespace Editor
//------------------------------------------------------------------------------
