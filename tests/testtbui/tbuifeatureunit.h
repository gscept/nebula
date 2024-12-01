#pragma once
//------------------------------------------------------------------------------
/**
    Tests::TBUIFeatureUnit

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/featureunit.h"
#include "game/manager.h"

namespace Tests
{

class TBUIFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(TBUIFeatureUnit)
    __DeclareSingleton(TBUIFeatureUnit)
public:
    ///
    TBUIFeatureUnit();
    ///
    ~TBUIFeatureUnit();

    ///
    void OnAttach() override;
    ///
    void OnActivate() override;
    ///
    void OnDeactivate() override;
    ///
    void OnBeginFrame() override;
};

} // namespace Tests




