#pragma once
//------------------------------------------------------------------------------
/**
    
    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "ids/id.h"
#include "ids/idallocator.h"
#include "streamnavmeshcache.h"

namespace Navigation
{

ID_32_24_8_NAMED_TYPE(CrowdAgentId, instance, navmeshId, generation, agentType);

class AgentContext 
{

public:
    /// constructor
    AgentContext();
    /// destructor
    virtual ~AgentContext();

    /// create context
    static void Create();

    /// set the target for an agent
    static void SetTarget(const Navigation::CrowdAgentId id, const Math::point& target);

    /// set the transform for a model
    static Math::point GetTarget(const Navigation::CrowdAgentId id);


        
private:

    typedef Ids::IdAllocator<
        CrowdAgentId,
        int,            // agentId in dtcrowds
        NavMeshId,      // navmesh this agent is connected to
        Math::point     // current target point
    > AgentContextAllocator;
    static AgentContextAllocator agentContextAllocator;
};

} // namespace Models
