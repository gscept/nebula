#pragma once
//------------------------------------------------------------------------------
/**
    Editor::LiveBatcher

    Provides a live asset exporting/batching utility for the editor.

    The batching runs the asset batcher as a process, one job at a time on a thread of its own, so
    the editor keeps drawing while an asset exports. What the batcher printed is kept as a log for
    the batcher panel. The exported files are reloaded on the main thread, by Update, because a
    resource can only be reloaded there.

    (C) 2025 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/path.h"
#include "threading/interlocked.h"
#include "util/string.h"

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
    /// Reload what the finished jobs exported. Called once per frame on the main thread
    static void Update();
    /// Batches all assets
    static void BatchAssets();
    /// Batch a single asset
    static void BatchAsset(const IO::Path& assetPath);
    /// Batch a single file
    static void BatchFile(const IO::Path& filePath);
    /// Batch a set of modes
    static void BatchModes(BatchModes modes);
    /// Wait for all batching to finish
    static void Wait();

    /// true while jobs are queued or running
    static bool IsBusy();
    /// what the batcher printed so far. A copy, because the batching thread appends to it
    static Util::String GetLog();
    /// a number that changes whenever the log does, so a reader copies it only then
    static uint GetLogVersion();
    /// forget what the batcher printed
    static void ClearLog();

    static Threading::Interlocked::AtomicCounter WorkCounter;

};

} // namespace Editor
