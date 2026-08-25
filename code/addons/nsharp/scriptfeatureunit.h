#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::ScriptFeatureUnit

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "assemblyid.h"
#include "threading/safequeue.h"
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Scripting
{

class NSharpServer;

class ScriptFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(ScriptFeatureUnit)
    __DeclareSingleton(ScriptFeatureUnit)

public:

    /// constructor
    ScriptFeatureUnit();
    /// destructor
    ~ScriptFeatureUnit();
    
    /// call this before attaching the feature unit
    void SetScriptAssembly(IO::URI const& path);
    Scripting::AssemblyId GetAssemblyId() const;

    void OnActivate() override;
    void OnDeactivate() override;
    void OnBeginFrame() override;
    void OnFrame() override;
    void OnEndFrame() override;
    virtual void OnRenderDebug();

private:
    IO::URI engineAssemblyPath;
    IO::URI assemblyPath;
    Scripting::AssemblyId engineAssemblyId;
    Ptr<Scripting::NSharpServer> server;
    Threading::SafeQueue<Util::String> reloadQueue;
    Util::Array<Util::String> reloads;
    Util::String reloadWatchPath;
};

} // namespace Scripting
//------------------------------------------------------------------------------
