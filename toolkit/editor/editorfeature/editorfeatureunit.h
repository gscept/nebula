#pragma once
//------------------------------------------------------------------------------
/**
    EditorFeature::EditorFeatureUnit

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "core/ptr.h"
#include "game/featureunit.h"

namespace Audio
{
class AudioServer;
}

namespace EditorFeature
{
class EditorFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(EditorFeatureUnit);
    __DeclareSingleton(EditorFeatureUnit);
public:
    EditorFeatureUnit();
    ~EditorFeatureUnit();

    virtual void OnAttach();

    /// Called upon activation of feature unit
    virtual void OnActivate();
    /// Called upon deactivation of feature unit
    virtual void OnDeactivate();

    /// called at the end of the feature trigger cycle
    virtual void OnEndFrame();
    /// called when game debug visualization is on
    virtual void OnRenderDebug();
    /// called each frame
    virtual void OnFrame();

private:
    Ptr<Audio::AudioServer> audioServer;
    bool ownsAudioServer = false;
};

} // namespace Editor
