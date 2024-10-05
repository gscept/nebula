#pragma once
//------------------------------------------------------------------------------
/**
    Brute force system

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "visibilitysystem.h"
#include "jobs/jobs.h"
namespace Visibility
{

class BruteforceSystem : public VisibilitySystem
{
private:
    friend class ObserverContext;

    /// setup from load info
    void Setup(const BruteforceSystemLoadInfo& info);

    /// run system
    void Run(const Threading::AtomicCounter* previousSystemCompletionCounters, const Util::FixedArray<const Threading::AtomicCounter*, true>& extraCounters) override;
};

} // namespace Visibility
