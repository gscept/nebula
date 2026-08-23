//------------------------------------------------------------------------------
//  audiofeature/audiofeatureunit.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "scriptfeatureunit.h"
#include "nsharpserver.h"

namespace Scripting
{
__ImplementClass(Scripting::ScriptFeatureUnit, 'SCFU', Game::FeatureUnit);
__ImplementSingleton(ScriptFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
ScriptFeatureUnit::ScriptFeatureUnit()
    : engineAssemblyPath("bin:NebulaEngine.dll")
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
#if _DEBUG
    this->server->SetDebuggingEnabled(true);
    this->server->WaitForDebuggerToConnect(false);
#endif
    this->server->Open();

    if (this->assemblyPath.IsValid())
    {
        this->engineAssemblyId = this->server->LoadAssembly(this->engineAssemblyPath);
        n_assert(this->engineAssemblyId != Scripting::AssemblyId::Invalid());

        Util::String pluginPath = this->assemblyPath.GetHostAndLocalPath();
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
