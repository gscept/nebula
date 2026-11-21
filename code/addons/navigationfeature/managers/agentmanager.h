#pragma once
//------------------------------------------------------------------------------
/**
    @class  NavigationFeature::AgentManager

    Moves the entities with a NavAgent component over the navmeshes of the level.

    Every loaded navmesh which has agents gets a Detour crowd, which steers its agents
    along their paths and around each other. An agent joins the crowd of the navmesh
    baked for its agent kind as soon as that navmesh is loaded, a target set before
    that is walked to once it has joined.

    Each frame the crowd is stepped and the entity is moved:
    - with a Game::Velocity component, the velocity is set and whatever moves the entity
      by it does, like a physics character controller. The crowd follows the entity, so
      collisions are respected
    - without one, the position is set to the position on the navmesh

    A crowd is made for the agents of its kind there are, with room to spare. When more
    join than fit, it is made again for all of them, and its agents join again keeping
    their targets.

    The crowds step in jobs, off the game thread. The update starts in OnFrame, joining
    agents, following their entities and handing out paths, and dispatches a job for
    every crowd. It finishes in OnEndFrame, before the physics frame, waiting for the
    jobs and moving the entities. In between the crowds belong to the jobs: a target set
    meanwhile is put on the navmesh right away and handed to the crowd with the next
    update, and anything which changes or frees a crowd waits for the jobs first.

    A crowd keeps pointers into its navmesh, it is rebuilt when the navmesh is reloaded,
    and its agents join again, keeping their targets.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/world.h"
#include "math/point.h"
#include "navigationfeature/streamnavmeshcache.h"
#include "threading/event.h"

class dtCrowd;
class dtNavMesh;

namespace NavigationFeature
{

struct NavAgent;

enum class MoveState
{
    /// no target
    Idle,
    /// walking to the target
    Moving,
    /// at the target
    Arrived,
    /// the target can't be reached, the agent stopped at the closest point it can reach
    Unreachable,
    /// there is no navmesh near the target, or the agent isn't on its navmesh
    Failed,
    /// there is a target, but no navmesh is loaded for the agent kind yet
    NoNavMesh
};

class AgentManager : public Game::Manager
{
    __DeclareClass(AgentManager)
    __DeclareSingleton(AgentManager);
public:
    /// constructor
    AgentManager();
    /// destructor
    ~AgentManager();

    /// listen for navmesh reloads
    void OnActivate() override;
    /// free every crowd
    void OnDeactivate() override;
    /// start the update of the agents of the default world, the crowds step in jobs
    void OnFrame() override;
    /// finish the update, wait for the crowds and move the entities
    void OnEndFrame() override;
    /// free the crowds of a world which is cleaned up
    void OnCleanup(Game::World* world) override;

    /// register the agent of a NavAgent component
    static void InitAgent(Game::World* world, Game::Entity entity, NavAgent* component);
    /// remove the agent of a NavAgent component
    static void DecayAgent(Game::World* world, Game::Entity entity, NavAgent* component);

    /// join agents to crowds, step the crowds by dt and move the entities. A dt of 0 joins and requests without moving.
    /// Starts and finishes the update in one go
    static void UpdateAgents(Game::World* world, float dt);
    /// join agents to crowds, follow the entities, hand out paths, and start the crowds stepping by dt in jobs
    static void StartUpdate(Game::World* world, float dt);
    /// wait for the crowds of the update started last, and move the entities
    static void FinishUpdate(Game::World* world);
    /// wait until no crowd steps in a job, before a crowd or its navmesh is changed or freed
    static void WaitForCrowds();

    struct AgentRecord;
    struct CrowdRecord;
private:
    friend struct AgentAccess;
    Util::Array<AgentRecord*> agents;
    Util::Array<CrowdRecord*> crowds;

    /// the crowds the jobs step, signalled when the last is done
    Util::Array<dtCrowd*> stepping;
    Threading::Event steppingDone;
    bool crowdsStepping = false;
    /// the update which was started and not finished, and how far it steps
    Game::World* startedWorld = nullptr;
    float startedDt = 0.0f;
};

/// walk to a target. Returns false if the entity has no NavAgent
bool SetMoveTarget(Game::World* world, Game::Entity entity, const Math::point& target);
/// stop where the agent is
void StopMoving(Game::World* world, Game::Entity entity);
/// what the agent is doing, Idle if the entity has no NavAgent
MoveState GetMoveState(Game::World* world, Game::Entity entity);
/// the velocity the crowd moves the agent with, zero if it isn't moving
Math::vec3 GetAgentVelocity(Game::World* world, Game::Entity entity);
/// the corners of the path ahead of the agent, the next few the crowd steers by
Util::Array<Math::point> GetAgentCorners(Game::World* world, Game::Entity entity);
/// draw the agents, their targets and the corners ahead of them
void DrawAgents();

} // namespace NavigationFeature
