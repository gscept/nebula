//------------------------------------------------------------------------------
//  audiofeature/audiofeatureunit.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "scriptfeatureunit.h"
#include "nsharpserver.h"

namespace Scripting
{
__ImplementClass(Scripting::ScriptFeatureUnit, 'AUFU', Game::FeatureUnit);
__ImplementSingleton(ScriptFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
ScriptFeatureUnit::ScriptFeatureUnit()
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
    return this->assemblyId;
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
        this->assemblyId = this->server->LoadAssembly(this->assemblyPath);
        n_assert(this->assemblyId != Scripting::AssemblyId::Invalid());

        this->server->ExecUnmanagedCall(assemblyId, "AppEntry::Main()");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnDeactivate()
{
    this->server->ExecUnmanagedCall(this->assemblyId, "AppEntry::Exit()");
    this->server->Close();
    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnBeginFrame()
{
    this->server->ExecUnmanagedCall(this->assemblyId, "AppEntry::OnBeginFrame()");
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnFrame()
{
    this->server->ExecUnmanagedCall(this->assemblyId, "AppEntry::OnFrame()");
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnEndFrame()
{
    this->server->ExecUnmanagedCall(this->assemblyId, "AppEntry::OnEndFrame()");
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
