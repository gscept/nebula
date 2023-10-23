#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::ScriptFeatureUnit

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "assemblyid.h"

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
    IO::URI assemblyPath;
    Scripting::AssemblyId assemblyId;

    Ptr<Scripting::NSharpServer> server;
};

} // namespace Scripting
//------------------------------------------------------------------------------
