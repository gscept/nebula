#pragma once
//------------------------------------------------------------------------------
/**
    Portal system

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "visibilitysystem.h"
#include "jobs/jobs.h"
namespace Visibility
{
    
class PortalSystem : public VisibilitySystem
{
public:
private:
    friend class ObserverContext;

    /// setup from load info
    void Setup(const PortalSystemLoadInfo& info);
};
} // namespace Visibility
