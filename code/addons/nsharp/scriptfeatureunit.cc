//------------------------------------------------------------------------------
//  audiofeature/audiofeatureunit.cc
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
    engineAssemblyPath("bin:NebulaEngine.dll")
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

            IO::FileWatcher::Instance()->Watch(
                this->reloadWatchPath,
                false,
                IO::WatchFlags(IO::NameChanged | IO::SizeChanged | IO::Write | IO::Creation),
                reloadFileFunc
            );
        }

        int result = this->server->ExecUnmanagedCall(
            this->engineAssemblyId,
            "NebulaEngine.AppEntry::Main()",
            pluginPath
        );

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
    this->server->ExecUnmanagedCall(this->engineAssemblyId, "NebulaEngine.AppEntry::Exit()");
    this->server->Close();
    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnBeginFrame()
{
    this->reloadQueue.DequeueAll(this->reloads);

    if (!this->reloads.IsEmpty())
    {
        // Only reload the latest, clear the rest.
        this->server->ExecUnmanagedCall(
            this->engineAssemblyId,
            "NebulaEngine.AppEntry::RequestReload()",
            this->reloads[this->reloads.Size() - 1]
        );
        this->reloads.Clear();
    }

    this->server->ExecUnmanagedCall(this->engineAssemblyId, "NebulaEngine.AppEntry::OnBeginFrame()");
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnFrame()
{
    this->server->ExecUnmanagedCall(this->engineAssemblyId, "NebulaEngine.AppEntry::OnFrame()");
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnEndFrame()
{
    this->server->ExecUnmanagedCall(this->engineAssemblyId, "NebulaEngine.AppEntry::OnEndFrame()");
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
