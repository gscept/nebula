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

extern void BruteforceSystemJobFunc(const Jobs::JobFuncContext& ctx);

class BruteforceSystem : public VisibilitySystem
{
private:
    friend class ObserverContext;

    /// setup from load info
    void Setup(const BruteforceSystemLoadInfo& info);

    /// run system
    void Run(Threading::Event* previousSystemEvent) override;
};

} // namespace Visibility
