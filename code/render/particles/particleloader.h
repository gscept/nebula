#pragma once
//------------------------------------------------------------------------------
/**
    The loader for particle effects

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "coregraphics/config.h"

namespace Particles
{

class ParticleLoader : public Resources::ResourceLoader
{
    __DeclareClass(ParticleLoader);

public:
        /// setup resource loader, initiates the placeholder and error resources if valid, so don't forget to run!
    virtual void Setup() override;

    /// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
    ResourceInitOutput InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream) override;
private:

    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;
};

} // namespace Particles
