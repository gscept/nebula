#pragma once
//------------------------------------------------------------------------------
/**
    Editor::LiveBatcher

    Provides a live asset exporting/batching utility for the editor

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

namespace Editor
{

enum BatchModes
{
    Meshes = 1 << 0,
    Models = 1 << 1,
    Textures = 1 << 2,
};
__ImplementEnumBitOperators(BatchModes)

class LiveBatcher
{
public:

    /// Setup live batcher
    static void Setup();
    /// Shutdown live batcher
    static void Discard();
    /// Batches all assets
    static void BatchAssets();
    /// Batch a single asset
    static void BatchAsset(const IO::URI& assetPath);
    /// Batch a single file
    static void BatchFile(const IO::URI& filePath);
    /// Batch a set of modes
    static void BatchModes(BatchModes modes);
    /// Wait for all batching to finish
    static void Wait();
};

} // namespace Editor

namespace Presentation
{
class LiveBatcherWindow : public BaseWindow
{
    __DeclareClass(LiveBatcherWindow)

public:

    void Run(SaveMode save) override;
    void Update();
};

} // namespace Presentation