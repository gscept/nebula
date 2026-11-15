//------------------------------------------------------------------------------
//  nsharp/scriptfeatureunit.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "scriptfeatureunit.h"
#include "nsharpserver.h"
#include "io/filewatcher.h"

namespace Scripting
{
__ImplementClass(Scripting::ScriptFeatureUnit, 'SCFU', Game::FeatureUnit);
__ImplementSingleton(ScriptFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
ScriptFeatureUnit::ScriptFeatureUnit() :
    engineAssemblyPath("bin:NebulaEngine.dll"),
    mainEntry("NebulaEngine.AppEntry::Main()"),
    exitEntry("NebulaEngine.AppEntry::Exit()"),
    beginFrameEntry("NebulaEngine.AppEntry::OnBeginFrame()"),
    frameEntry("NebulaEngine.AppEntry::OnFrame()"),
    endFrameEntry("NebulaEngine.AppEntry::OnEndFrame()"),
    reloadEntry("NebulaEngine.AppEntry::RequestReload()")
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ScriptFeatureUnit::~ScriptFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::SetScriptAssembly(IO::URI const& path)
{
    this->assemblyPath = path;
}

//------------------------------------------------------------------------------
/**
*/
Scripting::AssemblyId
ScriptFeatureUnit::GetAssemblyId() const
{
    return this->engineAssemblyId;
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();
    this->server = NSharpServer::Create();
    this->server->Open();

    if (this->assemblyPath.IsValid())
    {
        this->engineAssemblyId = this->server->LoadAssembly(this->engineAssemblyPath);
        n_assert(this->engineAssemblyId != Scripting::AssemblyId::Invalid());

        Util::String pluginPath = this->assemblyPath.GetHostAndLocalPath();
        this->reloadWatchPath = pluginPath.ExtractDirName();
        Util::String reloadFileName = pluginPath.ExtractFileName();

        if (IO::FileWatcher::HasInstance())
        {
            auto reloadFileFunc = [this, reloadFileName](IO::WatchEvent const& event)
            {
                if ((event.type == IO::Modified || event.type == IO::NameChange || event.type == IO::Created) &&
                    event.file == reloadFileName)
                    this->reloadQueue.Enqueue(this->assemblyPath.GetHostAndLocalPath());
            };

            // WatchFlags values are bit positions, so they have to be set as bits, not or'd
            // together as a mask.
            Util::BitField<8> watchFlags;
            watchFlags.SetBit(IO::WatchFlags::NameChanged);
            watchFlags.SetBit(IO::WatchFlags::SizeChanged);
            watchFlags.SetBit(IO::WatchFlags::Write);
            watchFlags.SetBit(IO::WatchFlags::Creation);

            IO::FileWatcher::Instance()->Watch(
                this->reloadWatchPath,
                false,
                watchFlags,
                reloadFileFunc
            );
        }

        int result = this->server->ExecUnmanagedCall(this->engineAssemblyId, this->mainEntry, pluginPath);

        n_assert(result == 0);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnDeactivate()
{
    if (IO::FileWatcher::HasInstance() && !this->reloadWatchPath.IsEmpty())
    {
        IO::FileWatcher::Instance()->Unwatch(this->reloadWatchPath);
    }
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
    {
        this->server->Close();
        FeatureUnit::OnDeactivate();
        return;
    }

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->exitEntry);
    this->server->Close();
    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnBeginFrame()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->reloadQueue.DequeueAll(this->reloads);

    if (!this->reloads.IsEmpty())
    {
        // Only reload the latest, clear the rest.
        this->server->ExecUnmanagedCall(
            this->engineAssemblyId,
            this->reloadEntry,
            this->reloads[this->reloads.Size() - 1]
        );
        this->reloads.Clear();
    }

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->beginFrameEntry);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnFrame()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->frameEntry);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnEndFrame()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->endFrameEntry);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnRenderDebug()
{
    // TODO: implement me!
}

} // namespace Scripting
