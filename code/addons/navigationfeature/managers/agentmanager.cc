//------------------------------------------------------------------------------
//  agentmanager.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "agentmanager.h"
#include "navigationmanager.h"
#include "components/navigation.h"
#include "navigationfeature/navquery.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/velocity.h"
#include "game/timesource.h"
#include "basegamefeature/managers/timemanager.h"
#include "dynui/im3d/im3dcontext.h"
#include "DetourCrowd.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"
#include "DetourPathQueue.h"
#include "profiling/profiling.h"
#include "jobs2/jobs2.h"

namespace Navigation
{
extern StreamNavMeshCache* navMeshCache;
}

namespace NavigationFeature
{
__ImplementClass(NavigationFeature::AgentManager, 'NvAg', Game::Manager);
__ImplementSingleton(AgentManager)

/// the fewest agents a crowd is made for. A Detour crowd can't grow, it is made again with room for
/// every agent of its kind when one doesn't fit
static constexpr int MinCrowdAgents = 64;
/// how many paths are searched per update. A target gets its whole path from a full search, the crowd's own
/// queue only runs 100 search iterations per update for all of its agents, which many agents with far away
/// targets wait on for seconds. The searches left wait for the next update, the oldest first
static constexpr int MaxSearchesPerUpdate = 16;
/// the longest corridor of a Detour crowd, a longer path is planned further as the agent nears its end
static constexpr int MaxCorridorPolys = 256;

/// the searches the current update has left, a target set between updates uses them up too
static int searchesLeft = MaxSearchesPerUpdate;
/// hands out the order targets were set in, so the searches which wait are done oldest first
static uint64_t requestCounter = 0;

struct AgentManager::CrowdRecord
{
    Game::World* world = nullptr;
    Navigation::NavMeshId navMesh = Navigation::InvalidNavMeshId;
    /// the navmesh the crowd was made for, it is stale once the navmesh of the id is another
    const dtNavMesh* mesh = nullptr;
    dtCrowd* crowd = nullptr;
    /// how many agents the crowd was made for
    int capacity = 0;
    /// an agent didn't fit, the crowd is made again bigger with the next update
    bool grow = false;
};

struct AgentManager::AgentRecord
{
    Game::World* world = nullptr;
    Game::Entity entity;
    CrowdRecord* crowd = nullptr;
    int index = -1;
    bool hasTarget = false;
    /// the target was handed to the crowd
    bool requested = false;
    /// the target is on the navmesh, its path waits for a search
    bool searching = false;
    /// when the waiting search was asked for
    uint64_t requestOrder = 0;
    /// the target on the navmesh
    dtPolyRef targetRef = 0;
    float targetPos[3] = { 0.0f, 0.0f, 0.0f };
    Math::point target;
    MoveState state = MoveState::Idle;
    /// the velocity of the last step, read while the crowd steps
    Math::vec3 velocity = Math::vec3(0.0f);
    /// stopped while the crowd stepped, the crowd stops it when the update finishes
    bool stopPending = false;
};

/// what the feature unit's debug draw and the free functions reach into
struct AgentAccess
{
    static Util::Array<AgentManager::AgentRecord*>&
    Agents()
    {
        return AgentManager::Singleton->agents;
    }
    static Util::Array<AgentManager::CrowdRecord*>&
    Crowds()
    {
        return AgentManager::Singleton->crowds;
    }
    /// true while crowds step in jobs, they belong to the jobs until the update finishes
    static bool
    Stepping()
    {
        return AgentManager::Singleton->crowdsStepping;
    }
};

//------------------------------------------------------------------------------
/**
*/
AgentManager::AgentManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
AgentManager::~AgentManager()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
static AgentManager::AgentRecord*
FindAgent(Game::World* world, Game::Entity entity)
{
    if (!AgentManager::HasInstance())
    {
        return nullptr;
    }
    for (AgentManager::AgentRecord* agent : AgentAccess::Agents())
    {
        if (agent->world == world && agent->entity == entity)
        {
            return agent;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
static void
LeaveCrowd(AgentManager::AgentRecord* agent)
{
    if (agent->crowd != nullptr && agent->index >= 0)
    {
        agent->crowd->crowd->removeAgent(agent->index);
    }
    agent->crowd = nullptr;
    agent->index = -1;
    // the target is handed to the next crowd it joins
    agent->requested = false;
    agent->searching = false;
}

//------------------------------------------------------------------------------
/**
    The agents of the crowd join again when its navmesh is back
*/
static void
FreeCrowd(IndexT crowdIndex)
{
    Util::Array<AgentManager::CrowdRecord*>& crowds = AgentAccess::Crowds();
    AgentManager::CrowdRecord* crowd = crowds[crowdIndex];
    for (AgentManager::AgentRecord* agent : AgentAccess::Agents())
    {
        if (agent->crowd == crowd)
        {
            // the crowd goes away as a whole, no need to remove the agents one by one
            agent->crowd = nullptr;
            agent->index = -1;
            agent->requested = false;
            agent->searching = false;
        }
    }
    dtFreeCrowd(crowd->crowd);
    delete crowd;
    crowds.EraseIndex(crowdIndex);
}

//------------------------------------------------------------------------------
/**
    A crowd is stale when its navmesh was unloaded or reloaded, the crowd points into the old one
*/
static bool
CrowdIsCurrent(const AgentManager::CrowdRecord* crowd)
{
    if (Navigation::navMeshCache == nullptr)
    {
        return false;
    }
    for (Navigation::NavMeshId loaded : Navigation::navMeshCache->GetLoadedMeshes())
    {
        if (loaded == crowd->navMesh)
        {
            return Navigation::navMeshCache->GetDetourMesh(loaded) == crowd->mesh;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
static AgentManager::CrowdRecord*
GetCrowd(Game::World* world, Navigation::NavMeshId navMesh)
{
    for (AgentManager::CrowdRecord* crowd : AgentAccess::Crowds())
    {
        if (crowd->world == world && crowd->navMesh == navMesh)
        {
            return crowd;
        }
    }

    N_SCOPE(NavCreateCrowd, Navigation);
    dtNavMesh* mesh = Navigation::navMeshCache->GetDetourMesh(navMesh);
    const Navigation::AgentT& kind = Navigation::navMeshCache->GetAgent(navMesh);

    // room for every agent of the kind in the world and a quarter more, so a few more don't make it again
    const Util::StringAtom kindName(kind.name.c_str());
    int agentCount = 0;
    for (AgentManager::AgentRecord* agent : AgentAccess::Agents())
    {
        if (agent->world == world && world->HasComponent<NavAgent>(agent->entity) && world->GetComponent<NavAgent>(agent->entity).agentKind == kindName)
        {
            agentCount++;
        }
    }
    int capacity = MinCrowdAgents;
    while (capacity < agentCount + agentCount / 4 + 1)
    {
        capacity *= 2;
    }

    dtCrowd* detourCrowd = dtAllocCrowd();
    if (detourCrowd == nullptr || !detourCrowd->init(capacity, Math::max(kind.agent_radius, 0.01f), mesh))
    {
        n_warning("AgentManager: could not create a crowd for navmesh '%s'\n", Navigation::navMeshCache->GetName(navMesh).Value());
        dtFreeCrowd(detourCrowd);
        return nullptr;
    }

    // the costs of the area table, every flag
    const QueryFilter source = DefaultQueryFilter();
    dtQueryFilter* filter = detourCrowd->getEditableFilter(0);
    for (int i = 0; i < 64; i++)
    {
        filter->setAreaCost(i, source.areaCost[i]);
    }
    filter->setIncludeFlags(source.includeFlags);
    filter->setExcludeFlags(source.excludeFlags);

    AgentManager::CrowdRecord* crowd = new AgentManager::CrowdRecord();
    crowd->world = world;
    crowd->navMesh = navMesh;
    crowd->mesh = mesh;
    crowd->crowd = detourCrowd;
    crowd->capacity = capacity;
    AgentAccess::Crowds().Append(crowd);
    if (capacity > MinCrowdAgents)
    {
        n_printf("AgentManager: crowd of navmesh '%s' made for %d agents\n", Navigation::navMeshCache->GetName(navMesh).Value(), capacity);
    }
    return crowd;
}

//------------------------------------------------------------------------------
/**
*/
static bool
JoinCrowd(AgentManager::AgentRecord* agent, const NavAgent& component)
{
    const Navigation::NavMeshId navMesh = FindNavMesh(agent->world, component.agentKind);
    if (navMesh == Navigation::InvalidNavMeshId)
    {
        return false;
    }
    AgentManager::CrowdRecord* crowd = GetCrowd(agent->world, navMesh);
    if (crowd == nullptr)
    {
        return false;
    }

    const Navigation::AgentT& kind = Navigation::navMeshCache->GetAgent(navMesh);
    dtCrowdAgentParams params;
    memset(&params, 0, sizeof(params));
    params.radius = kind.agent_radius;
    params.height = kind.agent_height;
    params.maxSpeed = Math::max(component.maxSpeed, 0.0f);
    params.maxAcceleration = Math::max(component.maxAcceleration, 0.0f);
    params.collisionQueryRange = kind.agent_radius * 12.0f;
    params.pathOptimizationRange = kind.agent_radius * 30.0f;
    params.separationWeight = 2.0f;
    params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO |
        DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
    params.obstacleAvoidanceType = 0;
    params.queryFilterType = 0;

    const Math::vec3 position = agent->world->GetComponent<Game::Position>(agent->entity);
    const float pos[3] = { position.x, position.y, position.z };
    const int index = crowd->crowd->addAgent(pos, &params);
    if (index < 0)
    {
        // the agents which don't fit join the bigger crowd made with the next update
        crowd->grow = true;
        return false;
    }
    agent->crowd = crowd;
    agent->index = index;
    agent->requested = false;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::InitAgent(Game::World* world, Game::Entity entity, NavAgent*)
{
    if (!AgentManager::HasInstance())
    {
        return;
    }
    // a component which is replaced is initialized again, it keeps its target
    AgentRecord* agent = FindAgent(world, entity);
    if (agent != nullptr)
    {
        WaitForCrowds();
        LeaveCrowd(agent);
        return;
    }
    agent = new AgentRecord();
    agent->world = world;
    agent->entity = entity;
    Singleton->agents.Append(agent);
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::DecayAgent(Game::World* world, Game::Entity entity, NavAgent*)
{
    if (!AgentManager::HasInstance())
    {
        return;
    }
    for (IndexT i = 0; i < Singleton->agents.Size(); i++)
    {
        AgentRecord* agent = Singleton->agents[i];
        if (agent->world == world && agent->entity == entity)
        {
            WaitForCrowds();
            LeaveCrowd(agent);
            delete agent;
            Singleton->agents.EraseIndex(i);
            return;
        }
    }
}

//------------------------------------------------------------------------------
/**
    The whole path to the target, handed to the crowd as the agent's corridor
*/
static void
SearchPath(AgentManager::AgentRecord* agent)
{
    // summed up over the frame, there are up to MaxSearchesPerUpdate of them, and the ones of targets set between updates
    N_SCOPE_ACCUM(NavPathSearch, Navigation);
    dtCrowd* crowd = agent->crowd->crowd;
    dtCrowdAgent* crowdAgent = crowd->getEditableAgent(agent->index);
    dtNavMeshQuery* query = Navigation::navMeshCache->GetQuery(agent->crowd->navMesh);
    const dtQueryFilter* filter = crowd->getFilter(crowdAgent->params.queryFilterType);
    agent->searching = false;

    const dtPolyRef start = crowdAgent->corridor.getFirstPoly();
    dtPolyRef path[MaxCorridorPolys];
    int pathCount = 0;
    const dtStatus status = start != 0 && query != nullptr
        ? query->findPath(start, agent->targetRef, crowdAgent->npos, agent->targetPos, filter, path, &pathCount, MaxCorridorPolys)
        : DT_FAILURE;
    if (dtStatusFailed(status) || pathCount == 0)
    {
        // the crowd tries itself, from where the agent is
        crowd->requestMoveTarget(agent->index, agent->targetRef, agent->targetPos);
        return;
    }

    // a path which stops short of the target ends at the point of its last polygon closest to it. It is
    // only partial when the target can't be reached, a path which ran out of room or search nodes is
    // planned further by the crowd as the agent nears its end
    float end[3];
    dtVcopy(end, agent->targetPos);
    bool partial = false;
    if (path[pathCount - 1] != agent->targetRef)
    {
        query->closestPointOnPoly(path[pathCount - 1], agent->targetPos, end, nullptr);
        partial = !dtStatusDetail(status, DT_BUFFER_TOO_SMALL) && !dtStatusDetail(status, DT_OUT_OF_NODES);
    }

    // what the crowd sets when its own queue delivers a path
    crowdAgent->corridor.setCorridor(end, path, pathCount);
    crowdAgent->boundary.reset();
    crowdAgent->partial = partial;
    crowdAgent->targetRef = agent->targetRef;
    dtVcopy(crowdAgent->targetPos, agent->targetPos);
    crowdAgent->targetPathqRef = DT_PATHQ_INVALID;
    crowdAgent->targetReplan = false;
    crowdAgent->targetReplanTime = 0.0f;
    crowdAgent->targetState = DT_CROWDAGENT_TARGET_VALID;
}

//------------------------------------------------------------------------------
/**
    Puts the target on the surface closest to it, and searches its path now, or with the next update
    when this one has searched enough
*/
static void
RequestTarget(AgentManager::AgentRecord* agent)
{
    dtCrowd* crowd = agent->crowd->crowd;
    const float target[3] = { agent->target.x, agent->target.y, agent->target.z };
    // a little more room than the crowd places agents with, a target is often clicked or picked a bit off
    const float* placement = crowd->getQueryHalfExtents();
    const float extents[3] = { placement[0] * 2.0f, placement[1] * 2.0f, placement[2] * 2.0f };
    dtPolyRef ref = 0;
    float nearest[3];
    agent->requested = true;
    // the query of the navmesh, not the one of the crowd, which its job uses while it steps
    dtNavMeshQuery* query = Navigation::navMeshCache->GetQuery(agent->crowd->navMesh);
    if (query == nullptr || dtStatusFailed(query->findNearestPoly(target, extents, crowd->getFilter(0), &ref, nearest)) || ref == 0)
    {
        agent->state = MoveState::Failed;
        agent->hasTarget = false;
        agent->searching = false;
        return;
    }
    agent->state = MoveState::Moving;
    agent->targetRef = ref;
    dtVcopy(agent->targetPos, nearest);
    agent->searching = true;
    agent->requestOrder = ++requestCounter;
    if (AgentAccess::Stepping())
    {
        // the crowd belongs to its job, the search waits for the next update
        return;
    }
    if (searchesLeft > 0)
    {
        searchesLeft--;
        SearchPath(agent);
    }
    else
    {
        // it stands until its path is searched, rather than walk on to the target it had
        crowd->resetMoveTarget(agent->index);
    }
}

//------------------------------------------------------------------------------
/**
    The crowd follows the entity, which may have been moved by physics or the editor
*/
static void
FollowEntity(AgentManager::AgentRecord* agent, const NavAgent& component)
{
    dtCrowd* crowd = agent->crowd->crowd;
    dtCrowdAgent* crowdAgent = crowd->getEditableAgent(agent->index);
    const Math::vec3 position = agent->world->GetComponent<Game::Position>(agent->entity);
    const float pos[3] = { position.x, position.y, position.z };
    const float distance = dtVdist2D(pos, crowdAgent->npos);
    if (distance < 0.001f && Math::abs(pos[1] - crowdAgent->npos[1]) < crowdAgent->params.height)
    {
        return;
    }

    // a jump further than the agent is wide is a teleport, the agent joins again where the entity is
    if (distance > crowdAgent->params.radius || crowdAgent->state != DT_CROWDAGENT_STATE_WALKING)
    {
        const bool target = agent->hasTarget;
        LeaveCrowd(agent);
        if (JoinCrowd(agent, component) && target)
        {
            RequestTarget(agent);
        }
        return;
    }
    crowdAgent->corridor.movePosition(pos, const_cast<dtNavMeshQuery*>(crowd->getNavMeshQuery()), crowd->getFilter(crowdAgent->params.queryFilterType));
    dtVcopy(crowdAgent->npos, crowdAgent->corridor.getPos());
}

//------------------------------------------------------------------------------
/**
*/
static void
StopAgent(AgentManager::AgentRecord* agent, MoveState state)
{
    agent->state = state;
    agent->hasTarget = false;
    agent->searching = false;
    agent->stopPending = false;
    agent->velocity = Math::vec3(0.0f);
    if (agent->crowd != nullptr)
    {
        agent->crowd->crowd->resetMoveTarget(agent->index);
        dtCrowdAgent* crowdAgent = agent->crowd->crowd->getEditableAgent(agent->index);
        dtVset(crowdAgent->vel, 0, 0, 0);
        dtVset(crowdAgent->dvel, 0, 0, 0);
        dtVset(crowdAgent->nvel, 0, 0, 0);
    }
    if (agent->world->HasComponent<Game::Velocity>(agent->entity))
    {
        Game::Velocity velocity = agent->world->GetComponent<Game::Velocity>(agent->entity);
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        agent->world->SetComponent<Game::Velocity>(agent->entity, velocity);
    }
}

//------------------------------------------------------------------------------
/**
*/
static void
CheckArrival(AgentManager::AgentRecord* agent, const NavAgent& component)
{
    if (agent->state != MoveState::Moving)
    {
        return;
    }
    const dtCrowdAgent* crowdAgent = agent->crowd->crowd->getAgent(agent->index);
    if (crowdAgent->targetState == DT_CROWDAGENT_TARGET_FAILED)
    {
        StopAgent(agent, MoveState::Failed);
        return;
    }
    if (crowdAgent->targetState != DT_CROWDAGENT_TARGET_VALID)
    {
        return;
    }

    // the end of the corridor is the target, or the point closest to it the agent can reach
    const float* end = crowdAgent->corridor.getTarget();
    const float arrival = Math::max(component.arrivalDistance, 0.01f);
    if (dtVdist2D(crowdAgent->npos, end) <= arrival && Math::abs(crowdAgent->npos[1] - end[1]) < crowdAgent->params.height)
    {
        StopAgent(agent, crowdAgent->partial ? MoveState::Unreachable : MoveState::Arrived);
    }
}

//------------------------------------------------------------------------------
/**
    Turns the +z axis of the entity towards where it walks, around the up axis
*/
static void
TurnTowards(Game::World* world, Game::Entity entity, const float* velocity, float turnSpeed, float dt)
{
    if (turnSpeed <= 0.0f || dt <= 0.0f || sqrtf(velocity[0] * velocity[0] + velocity[2] * velocity[2]) < 0.05f || !world->HasComponent<Game::Orientation>(entity))
    {
        return;
    }
    const Math::quat orientation = world->GetComponent<Game::Orientation>(entity);
    const Math::vec3 forward = Math::rotate(orientation, Math::vec3(0, 0, 1));
    const float current = atan2f(forward.x, forward.z);
    const float wanted = atan2f(velocity[0], velocity[2]);
    float delta = wanted - current;
    while (delta > N_PI)
    {
        delta -= 2.0f * N_PI;
    }
    while (delta < -N_PI)
    {
        delta += 2.0f * N_PI;
    }
    const float step = Math::clamp(delta, -turnSpeed * dt, turnSpeed * dt);
    world->SetComponent<Game::Orientation>(entity, Game::Orientation(Math::rotationquataxis(Math::vec3(0, 1, 0), current + step)));
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::UpdateAgents(Game::World* world, float dt)
{
    StartUpdate(world, dt);
    FinishUpdate(world);
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::WaitForCrowds()
{
    if (!AgentManager::HasInstance() || !Singleton->crowdsStepping)
    {
        return;
    }
    // how long the game thread waits on the jobs, nothing when they finished during the frame
    N_SCOPE(NavCrowdWait, Navigation);
    Singleton->steppingDone.Wait();
    Singleton->crowdsStepping = false;
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::StartUpdate(Game::World* world, float dt)
{
    if (!AgentManager::HasInstance() || Navigation::navMeshCache == nullptr)
    {
        return;
    }

    N_SCOPE(NavAgentsStart, Navigation);
    // an update which was started and not finished is finished first
    if (Singleton->startedWorld != nullptr)
    {
        FinishUpdate(Singleton->startedWorld);
    }
    searchesLeft = MaxSearchesPerUpdate;

    // crowds of navmeshes which were unloaded or reloaded go, and ones which ran out of room. Their agents
    // join again below, the ones of a full crowd join one made for all of them
    for (IndexT i = Singleton->crowds.Size() - 1; i >= 0; i--)
    {
        CrowdRecord* crowd = Singleton->crowds[i];
        if (crowd->world == world && (crowd->grow || !CrowdIsCurrent(crowd)))
        {
            FreeCrowd(i);
        }
    }

    // agents join crowds, crowds follow their entities, new targets are put on the navmesh
    N_MARKER_BEGIN(NavAgentsSync, Navigation);
    for (AgentRecord* agent : Singleton->agents)
    {
        if (agent->world != world || !world->HasComponent<NavAgent>(agent->entity))
        {
            continue;
        }
        const NavAgent& component = world->GetComponent<NavAgent>(agent->entity);
        if (agent->crowd == nullptr && !JoinCrowd(agent, component))
        {
            if (agent->hasTarget)
            {
                agent->state = MoveState::NoNavMesh;
            }
            continue;
        }
        FollowEntity(agent, component);
        if (agent->crowd != nullptr && agent->hasTarget && !agent->requested)
        {
            RequestTarget(agent);
        }
    }
    N_MARKER_END();

    // the searches which waited for an update with searches left, the oldest first
    {
        N_SCOPE(NavAgentsWaitingSearches, Navigation);
        Util::Array<AgentRecord*> waiting;
        for (AgentRecord* agent : Singleton->agents)
        {
            if (agent->world == world && agent->searching && agent->crowd != nullptr)
            {
                waiting.Append(agent);
            }
        }
        waiting.SortWithFunc([](AgentRecord* const& a, AgentRecord* const& b)
        {
            return a->requestOrder < b->requestOrder;
        });
        for (IndexT i = 0; i < waiting.Size(); i++)
        {
            if (searchesLeft > 0)
            {
                searchesLeft--;
                SearchPath(waiting[i]);
            }
            else
            {
                // it stands until its path is searched, rather than walk on to the target it had
                waiting[i]->crowd->crowd->resetMoveTarget(waiting[i]->index);
            }
        }
    }

    // Detour steers the agents, avoids the others and moves them along their corridors, a job for every crowd.
    // The crowds of a navmesh share it, which they only read
    Singleton->startedWorld = world;
    Singleton->startedDt = dt;
    Singleton->stepping.Clear();
    if (dt > 0.0f)
    {
        for (CrowdRecord* crowd : Singleton->crowds)
        {
            if (crowd->world == world)
            {
                Singleton->stepping.Append(crowd->crowd);
            }
        }
    }
    if (!Singleton->stepping.IsEmpty())
    {
        Singleton->crowdsStepping = true;
        dtCrowd* const* crowds = Singleton->stepping.Begin();
        Jobs2::JobDispatch(
            [crowds, dt](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
            {
                for (SizeT i = 0; i < groupSize; i++)
                {
                    const SizeT index = invocationOffset + i;
                    if (index >= totalJobs)
                    {
                        return;
                    }
                    N_SCOPE(NavCrowdUpdate, Navigation);
                    crowds[index]->update(dt, nullptr);
                }
            },
            Singleton->stepping.Size(), 1, nullptr, nullptr, &Singleton->steppingDone);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::FinishUpdate(Game::World* world)
{
    if (!AgentManager::HasInstance() || Singleton->startedWorld != world)
    {
        return;
    }
    N_SCOPE(NavAgentsFinish, Navigation);
    WaitForCrowds();
    const float dt = Singleton->startedDt;
    Singleton->startedWorld = nullptr;

    // agents stopped while the crowds stepped
    for (AgentRecord* agent : Singleton->agents)
    {
        if (agent->world == world && agent->stopPending)
        {
            StopAgent(agent, agent->state);
        }
    }

    // the entities are moved, turned, and agents at their target stop
    N_MARKER_BEGIN(NavAgentsApply, Navigation);

    for (AgentRecord* agent : Singleton->agents)
    {
        if (agent->world != world || agent->crowd == nullptr || !world->HasComponent<NavAgent>(agent->entity))
        {
            continue;
        }
        const NavAgent& component = world->GetComponent<NavAgent>(agent->entity);
        const dtCrowdAgent* crowdAgent = agent->crowd->crowd->getAgent(agent->index);
        if (crowdAgent->state != DT_CROWDAGENT_STATE_WALKING)
        {
            agent->velocity = Math::vec3(0.0f);
            continue;
        }
        agent->velocity = Math::vec3(crowdAgent->vel[0], crowdAgent->vel[1], crowdAgent->vel[2]);

        if (dt > 0.0f)
        {
            if (world->HasComponent<Game::Velocity>(agent->entity))
            {
                // the crowd follows the entity next frame, wherever this velocity gets it
                Game::Velocity velocity = world->GetComponent<Game::Velocity>(agent->entity);
                velocity.x = crowdAgent->vel[0];
                velocity.z = crowdAgent->vel[2];
                world->SetComponent<Game::Velocity>(agent->entity, velocity);
            }
            else
            {
                world->SetComponent<Game::Position>(agent->entity, Game::Position(crowdAgent->npos[0], crowdAgent->npos[1], crowdAgent->npos[2]));
            }
            TurnTowards(world, agent->entity, crowdAgent->vel, component.turnSpeed, dt);
        }
        CheckArrival(agent, component);
    }
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
    A reloaded navmesh is checked by the next update, this makes sure a navmesh which is
    reloaded at the address of the old one isn't missed
*/
static void
OnNavMeshReloaded(Resources::ResourceId id)
{
    if (!AgentManager::HasInstance())
    {
        return;
    }
    AgentManager::WaitForCrowds();
    Util::Array<AgentManager::CrowdRecord*>& crowds = AgentAccess::Crowds();
    for (IndexT i = crowds.Size() - 1; i >= 0; i--)
    {
        if (crowds[i]->navMesh == Navigation::NavMeshId(id))
        {
            FreeCrowd(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::OnActivate()
{
    Game::Manager::OnActivate();
    if (Navigation::navMeshCache != nullptr)
    {
        Navigation::navMeshCache->AddNavMeshReloadListener(&OnNavMeshReloaded);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::OnDeactivate()
{
    WaitForCrowds();
    this->startedWorld = nullptr;
    while (!this->crowds.IsEmpty())
    {
        FreeCrowd(this->crowds.Size() - 1);
    }
    for (AgentRecord* agent : this->agents)
    {
        delete agent;
    }
    this->agents.Clear();
    Game::Manager::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
AgentManager::OnFrame()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::TimeSource* time = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    if (world != nullptr && time != nullptr)
    {
        StartUpdate(world, (float)time->frameTime);
    }
}

//------------------------------------------------------------------------------
/**
    Before the physics frame of the world, which moves the characters by the velocities set here
*/
void
AgentManager::OnEndFrame()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    if (world != nullptr)
    {
        FinishUpdate(world);
    }
}

//------------------------------------------------------------------------------
/**
    Removing the entities of a world doesn't decay their components
*/
void
AgentManager::OnCleanup(Game::World* world)
{
    WaitForCrowds();
    if (this->startedWorld == world)
    {
        this->startedWorld = nullptr;
    }
    for (IndexT i = this->crowds.Size() - 1; i >= 0; i--)
    {
        if (this->crowds[i]->world == world)
        {
            FreeCrowd(i);
        }
    }
    for (IndexT i = this->agents.Size() - 1; i >= 0; i--)
    {
        if (this->agents[i]->world == world)
        {
            delete this->agents[i];
            this->agents.EraseIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
SetMoveTarget(Game::World* world, Game::Entity entity, const Math::point& target)
{
    AgentManager::AgentRecord* agent = FindAgent(world, entity);
    if (agent == nullptr)
    {
        return false;
    }
    agent->target = target;
    agent->hasTarget = true;
    agent->requested = false;
    agent->state = MoveState::Moving;
    // in a crowd it is requested right away, the next update moves the agent
    if (agent->crowd != nullptr)
    {
        RequestTarget(agent);
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
StopMoving(Game::World* world, Game::Entity entity)
{
    AgentManager::AgentRecord* agent = FindAgent(world, entity);
    if (agent == nullptr)
    {
        return;
    }
    if (AgentAccess::Stepping() && agent->crowd != nullptr)
    {
        // the crowd stops it when the update finishes, the entity stops now
        agent->state = MoveState::Idle;
        agent->hasTarget = false;
        agent->searching = false;
        agent->stopPending = true;
        agent->velocity = Math::vec3(0.0f);
        if (world->HasComponent<Game::Velocity>(entity))
        {
            Game::Velocity velocity = world->GetComponent<Game::Velocity>(entity);
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            world->SetComponent<Game::Velocity>(entity, velocity);
        }
        return;
    }
    StopAgent(agent, MoveState::Idle);
}

//------------------------------------------------------------------------------
/**
*/
MoveState
GetMoveState(Game::World* world, Game::Entity entity)
{
    AgentManager::AgentRecord* agent = FindAgent(world, entity);
    return agent != nullptr ? agent->state : MoveState::Idle;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3
GetAgentVelocity(Game::World* world, Game::Entity entity)
{
    AgentManager::AgentRecord* agent = FindAgent(world, entity);
    if (agent == nullptr || agent->crowd == nullptr || agent->state != MoveState::Moving)
    {
        return Math::vec3(0.0f);
    }
    // the one of the last step, the crowd may be stepping in its job
    return agent->velocity;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Math::point>
GetAgentCorners(Game::World* world, Game::Entity entity)
{
    Util::Array<Math::point> corners;
    AgentManager::WaitForCrowds();
    AgentManager::AgentRecord* agent = FindAgent(world, entity);
    if (agent == nullptr || agent->crowd == nullptr || agent->state != MoveState::Moving)
    {
        return corners;
    }
    const dtCrowdAgent* crowdAgent = agent->crowd->crowd->getAgent(agent->index);
    for (int i = 0; i < crowdAgent->ncorners; i++)
    {
        corners.Append(Math::point(crowdAgent->cornerVerts[i * 3], crowdAgent->cornerVerts[i * 3 + 1], crowdAgent->cornerVerts[i * 3 + 2]));
    }
    return corners;
}

//------------------------------------------------------------------------------
/**
*/
void
DrawAgents()
{
    N_SCOPE(NavDrawAgents, Navigation);
    if (!AgentManager::HasInstance())
    {
        return;
    }
    AgentManager::WaitForCrowds();
    for (AgentManager::AgentRecord* agent : AgentAccess::Agents())
    {
        if (agent->crowd == nullptr)
        {
            continue;
        }
        const dtCrowdAgent* crowdAgent = agent->crowd->crowd->getAgent(agent->index);
        if (!crowdAgent->active)
        {
            continue;
        }
        const Math::point base(crowdAgent->npos[0], crowdAgent->npos[1], crowdAgent->npos[2]);
        const Math::point top = base + Math::vector(0.0f, crowdAgent->params.height, 0.0f);

        Math::vec4 color;
        switch (agent->state)
        {
            case MoveState::Moving:      color = Math::vec4(0.3f, 0.8f, 1.0f, 0.6f); break;
            case MoveState::Arrived:     color = Math::vec4(0.3f, 1.0f, 0.4f, 0.6f); break;
            case MoveState::Unreachable: color = Math::vec4(1.0f, 0.7f, 0.2f, 0.6f); break;
            case MoveState::Failed:      color = Math::vec4(1.0f, 0.25f, 0.2f, 0.6f); break;
            default:                     color = Math::vec4(0.8f, 0.8f, 0.8f, 0.6f); break;
        }
        if (crowdAgent->state != DT_CROWDAGENT_STATE_WALKING)
        {
            color = Math::vec4(1.0f, 0.0f, 1.0f, 0.6f);
        }
        Im3d::Im3dContext::DrawCylinder(base, top, crowdAgent->params.radius, color);

        // the velocity, and the corners ahead up to the target
        const Math::vector lift(0.0f, 0.1f, 0.0f);
        const Math::point velocityEnd = base + Math::vector(crowdAgent->vel[0], crowdAgent->vel[1], crowdAgent->vel[2]);
        Im3d::Im3dContext::DrawLine(Math::line(base + lift, velocityEnd + lift), 2.0f, Math::vec4(1.0f, 1.0f, 0.2f, 1.0f), Im3d::AlwaysOnTop);
        if (agent->state == MoveState::Moving)
        {
            Math::point previous = base;
            for (int i = 0; i < crowdAgent->ncorners; i++)
            {
                const Math::point corner(crowdAgent->cornerVerts[i * 3], crowdAgent->cornerVerts[i * 3 + 1], crowdAgent->cornerVerts[i * 3 + 2]);
                Im3d::Im3dContext::DrawLine(Math::line(previous + lift, corner + lift), 1.5f, Math::vec4(1.0f, 1.0f, 1.0f, 0.8f), Im3d::AlwaysOnTop);
                previous = corner;
            }
            Im3d::Im3dContext::DrawPoint(Math::vec3(agent->target.x, agent->target.y + 0.1f, agent->target.z), 12.0f, Math::vec4(1.0f, 0.3f, 0.2f, 1.0f), Im3d::AlwaysOnTop);
        }
    }
}

} // namespace NavigationFeature
