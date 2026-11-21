//------------------------------------------------------------------------------
//  navigationtest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "navigationtest.h"
#include "game/gameserver.h"
#include "game/world.h"
#include "navigationfeature/components/navigation.h"
#include "navigationfeature/managers/navigationmanager.h"
#include "navigationfeature/streamnavmeshcache.h"
#include "navigationfeature/navquery.h"
#include "physicsfeature/components/physicsfeature.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "physicsfeature/managers/charactermanager.h"
#include "physics/charactercontext.h"
#include "physicsinterface.h"
#include "navigationfeature/managers/agentmanager.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/velocity.h"
#include "game/timesource.h"
#include "basegamefeature/managers/timemanager.h"
#include "navbuild/navmeshasset.h"
#include "navbuild/navmeshbuilder.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/navigation/navmesh.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "physics/geometry.h"
#include "resources/resourceserver.h"
#include "io/ioserver.h"
#include "io/path.h"
#include "io/assignregistry.h"

using namespace NavigationFeature;

namespace Navigation
{
extern StreamNavMeshCache* navMeshCache;
}

namespace Test
{
__ImplementClass(Test::NavigationManagerTest, 'NVMT', Test::TestCase);

//------------------------------------------------------------------------------
/**
    Bakes a flat 20x20 quad for the agent kind into the export of the URN
*/
static bool
BakeQuad(const IO::Path& asset, const char* agentKind, bool island = false, const Math::vec3& offset = Math::vec3(0.0f))
{
    const Physics::GeometryLoader previous = Physics::GetGeometryLoader();
    Physics::GeometryLoader loader;
    loader.LoadMesh = [](Util::StringAtom resource, IndexT meshIndex, IndexT primGroup, Physics::MeshGeometryInput& out) -> bool
    {
        if (resource != Util::StringAtom("navtest:quad:msh") || meshIndex != 0 || primGroup != 0)
        {
            return false;
        }
        const float s = 20.0f;
        const float verts[] = { 0, 0, 0,   0, 0, s,   s, 0, 0,   s, 0, s };
        out.positions.AppendArray(verts, 12);
        out.vertexCount = 4;
        const uint32_t indices[] = { 0, 1, 2,   2, 1, 3 };
        out.indexType = Physics::IndexType::Index32;
        out.indices.AppendArray((const byte*)indices, sizeof(indices));
        out.indexCount = 6;
        return true;
    };
    Physics::SetGeometryLoader(loader);

    Navigation::NavMeshT description;
    description.name = asset.GetFile().AsCharPtr();
    description.agent_kind = std::make_unique<Navigation::AgentT>();
    description.agent_kind->name = agentKind;
    description.agent_kind->agent_height = 2.0f;
    description.agent_kind->agent_radius = 0.5f;
    description.agent_kind->agent_max_climb = 0.5f;
    description.navmesh_settings = std::make_unique<Navigation::NavMeshSettingsT>();
    auto mesh = std::make_unique<Navigation::GeometryEntryT>();
    Navigation::MeshSourceT meshSource;
    meshSource.resource = "navtest:quad:msh";
    mesh->geometry.Set(meshSource);
    mesh->transform = Math::translation(offset);
    description.sources.push_back(std::move(mesh));
    if (island)
    {
        // a second quad with a 10 unit gap, which no path reaches
        auto islandMesh = std::make_unique<Navigation::GeometryEntryT>();
        islandMesh->geometry.Set(meshSource);
        islandMesh->transform = Math::translation(30.0f, 0.0f, 0.0f);
        description.sources.push_back(std::move(islandMesh));
    }

    const IO::URI workFile("temp:navigationtest.nanav");
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<Navigation::NavMesh>(description);
    bool written = false;
    {
        Ptr<IO::Stream> stream = IO::CreateStream(workFile);
        stream->SetAccessMode(IO::Stream::WriteAccess);
        if (stream->Open())
        {
            stream->Write(data.GetPtr(), data.Size());
            stream->Close();
            written = true;
        }
    }

    const IO::URI exportFile = asset.ExportURI();
    IO::CreateDirectory(IO::URI("nav:navtest"));
    const bool baked = written && Navigation::Build::BakeNavMesh(workFile, exportFile, true) == Navigation::Build::BakeResult::Baked;
    Physics::SetGeometryLoader(previous);
    IO::DeleteFile(workFile);
    return baked;
}

//------------------------------------------------------------------------------
/**
    Component inits run when the adds are executed at the begin of the frame, decays when the removes are
*/
static void
StepFrame()
{
    Game::GameServer::Instance()->OnBeginFrame();
    Game::GameServer::Instance()->OnFrame();
    Game::GameServer::Instance()->OnEndFrame();
}

//------------------------------------------------------------------------------
/**
*/
static Game::Entity
CreateNavMeshEntity(Game::World* world, const char* resource)
{
    // the component is initialized when it is added, with the value it is added with
    Game::Entity const entity = world->CreateEntity();
    NavMesh navMesh;
    navMesh.resource = resource;
    world->AddComponent(entity, Game::GetComponentId<NavMesh>(), &navMesh);
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
static SizeT
ActiveCount(Game::World* world)
{
    SizeT count = 0;
    for (const ActiveNavMesh& navMesh : GetActiveNavMeshes())
    {
        count += navMesh.world == world ? 1 : 0;
    }
    return count;
}

//------------------------------------------------------------------------------
/**
*/
static bool
Near(float a, float b, float tolerance)
{
    return Math::abs(a - b) <= tolerance;
}

//------------------------------------------------------------------------------
/**
    Queries on a 20x20 quad with an island 10 units beside it, from 30 to 50 on x
*/
static void
RunQueries(TestCase* test, Navigation::NavMeshId navMesh)
{
#define QVERIFY(exp) test->Verify(exp, #exp, __FILE__, __LINE__)
    // the nearest point is on the surface, below the position
    Math::point nearest;
    QVERIFY(FindNearestPoint(navMesh, Math::point(5, 1.5f, 5), Math::vec3(1, 2, 1), nearest));
    QVERIFY(Near(nearest.x, 5.0f, 0.01f) && Near(nearest.y, 0.0f, 0.3f) && Near(nearest.z, 5.0f, 0.01f));
    QVERIFY(!FindNearestPoint(navMesh, Math::point(100, 0, 100), Math::vec3(1, 2, 1), nearest));

    // across the open quad the path is a straight line, on the default area with its cost
    const Math::point start(2, 0, 2), end(18, 0, 18);
    NavPath path = FindPath(navMesh, start, end);
    QVERIFY(path.status == PathStatus::Found);
    QVERIFY(path.points.Size() >= 2 && path.areas.Size() == path.points.Size() - 1);
    if (path.points.Size() >= 2)
    {
        QVERIFY(Near(path.points[0].x, 2.0f, 0.01f) && Near(path.points[0].z, 2.0f, 0.01f));
        QVERIFY(Near(path.points.Back().x, 18.0f, 0.01f) && Near(path.points.Back().z, 18.0f, 0.01f));
    }
    const float straight = Math::length(end - start);
    QVERIFY(Near(path.length, straight, 0.1f));
    QVERIFY(Near(path.cost, path.length, 0.01f));
    bool defaultArea = true;
    for (unsigned char area : path.areas)
    {
        defaultArea &= area == Navigation::Build::DefaultWalkableArea;
    }
    QVERIFY(defaultArea);

    // the cost of an area scales the cost of the path, not its length
    QueryFilter expensive = DefaultQueryFilter();
    QVERIFY(SetAreaCost(expensive, "default", 3.0f));
    QVERIFY(!SetAreaCost(expensive, "nosucharea", 3.0f));
    NavPath expensivePath = FindPath(navMesh, start, end, &expensive);
    QVERIFY(expensivePath.status == PathStatus::Found);
    QVERIFY(Near(expensivePath.cost, expensivePath.length * 3.0f, 0.05f));

    // the island can't be reached, the path ends at the edge closest to it
    NavPath partial = FindPath(navMesh, Math::point(10, 0, 10), Math::point(40, 0, 10));
    QVERIFY(partial.status == PathStatus::Partial);
    QVERIFY(partial.points.Size() >= 2 && partial.points.Back().x <= 20.0f && partial.points.Back().x > 18.0f);

    // no surface near either end
    QVERIFY(FindPath(navMesh, Math::point(100, 0, 100), end).status == PathStatus::NoStart);
    QVERIFY(FindPath(navMesh, start, Math::point(100, 0, 100)).status == PathStatus::NoEnd);

    // a filter which lets no polygon through finds nothing
    QueryFilter none = DefaultQueryFilter();
    none.includeFlags = 0;
    QVERIFY(FindPath(navMesh, start, end, &none).status == PathStatus::NoStart);
    QVERIFY(!RandomPoint(navMesh, nearest, &none));

    // a ray across the quad is clear, one leaving it stops at its edge
    Math::point hit;
    QVERIFY(!Raycast(navMesh, Math::point(5, 0, 5), Math::point(15, 0, 5), hit));
    QVERIFY(Raycast(navMesh, Math::point(5, 0, 5), Math::point(25, 0, 5), hit));
    QVERIFY(hit.x > 18.0f && hit.x <= 20.0f && Near(hit.z, 5.0f, 0.01f) && Near(hit.y, 0.0f, 0.3f));

    // random points are on one of the quads
    bool onSurface = true;
    bool onIsland = false;
    for (int i = 0; i < 200; i++)
    {
        Math::point p;
        if (!RandomPoint(navMesh, p))
        {
            onSurface = false;
            break;
        }
        const bool mainQuad = p.x >= 0.0f && p.x <= 20.0f;
        const bool islandQuad = p.x >= 30.0f && p.x <= 50.0f;
        onIsland |= islandQuad;
        onSurface &= (mainQuad || islandQuad) && p.z >= 0.0f && p.z <= 20.0f && Near(p.y, 0.0f, 0.3f);
    }
    QVERIFY(onSurface);
    QVERIFY(onIsland);

    // random points around a center stay on the surface connected to it
    bool connected = true;
    for (int i = 0; i < 50; i++)
    {
        Math::point p;
        connected &= RandomPointAround(navMesh, Math::point(10, 0, 10), 2.0f, p) && p.x >= 0.0f && p.x <= 20.0f;
    }
    QVERIFY(connected);

    // queries on a navmesh which isn't there find nothing
    QVERIFY(FindPath(Navigation::InvalidNavMeshId, start, end).status == PathStatus::Failed);
    QVERIFY(!FindNearestPoint(Navigation::InvalidNavMeshId, start, Math::vec3(1, 2, 1), nearest));
#undef QVERIFY
}

//------------------------------------------------------------------------------
/**
*/
static Game::Entity
CreateAgent(Game::World* world, const char* agentKind, const Math::vec3& position, bool velocity)
{
    Game::Entity const entity = world->CreateEntity();
    world->SetComponent<Game::Position>(entity, Game::Position(position.x, position.y, position.z));
    if (velocity)
    {
        world->AddComponent<Game::Velocity>(entity);
    }
    NavAgent agent;
    agent.agentKind = agentKind;
    world->AddComponent(entity, Game::GetComponentId<NavAgent>(), &agent);
    return entity;
}

//------------------------------------------------------------------------------
/**
    Steps the agents until the entity stops moving, at most the number of steps. With integrate, the
    test moves the entity by its velocity, the way a character controller would
*/
static MoveState
WalkUntilStopped(Game::World* world, Game::Entity entity, int maxSteps, bool integrate = false)
{
    const float dt = 0.05f;
    for (int i = 0; i < maxSteps && GetMoveState(world, entity) == MoveState::Moving; i++)
    {
        AgentManager::UpdateAgents(world, dt);
        if (integrate)
        {
            Math::vec3 position = world->GetComponent<Game::Position>(entity);
            position += Math::vec3(world->GetComponent<Game::Velocity>(entity)) * dt;
            world->SetComponent<Game::Position>(entity, Game::Position(position.x, position.y, position.z));
        }
    }
    return GetMoveState(world, entity);
}

//------------------------------------------------------------------------------
/**
    Agents on the walker navmesh, a 20x20 quad with an island from 30 to 50 on x
*/
static void
RunAgents(TestCase* test, Game::World* world)
{
#define QVERIFY(exp) test->Verify(exp, #exp, __FILE__, __LINE__)
    // the gameplay clock stands still, the test steps the agents itself
    Game::TimeSource* gameplay = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    const float timeFactor = gameplay->timeFactor;
    gameplay->timeFactor = 0.0f;

    Game::Entity navMeshEntity = CreateNavMeshEntity(world, "navtest:walker:nav");
    Game::Entity const walker = CreateAgent(world, "walker", Math::vec3(2, 0, 2), false);
    world->SetComponent<Game::Orientation>(walker, Game::Orientation(Math::quat()));
    StepFrame();
    QVERIFY(GetMoveState(world, walker) == MoveState::Idle);

    // an agent without a velocity is placed on the navmesh, it arrives and faces where it walked
    QVERIFY(SetMoveTarget(world, walker, Math::point(18, 0, 2)));
    QVERIFY(GetMoveState(world, walker) == MoveState::Moving);
    AgentManager::UpdateAgents(world, 0.05f);
    QVERIFY(GetAgentVelocity(world, walker).x > 0.0f);
    QVERIFY(!GetAgentCorners(world, walker).IsEmpty());
    QVERIFY(WalkUntilStopped(world, walker, 400) == MoveState::Arrived);
    Math::vec3 position = world->GetComponent<Game::Position>(walker);
    QVERIFY(Near(position.x, 18.0f, 0.3f) && Near(position.z, 2.0f, 0.3f) && Near(position.y, 0.0f, 0.3f));
    const Math::vec3 forward = Math::rotate(world->GetComponent<Game::Orientation>(walker), Math::vec3(0, 0, 1));
    QVERIFY(forward.x > 0.9f);
    QVERIFY(GetAgentVelocity(world, walker).x == 0.0f);

    // the island can't be reached, the agent stops at the edge closest to it
    QVERIFY(SetMoveTarget(world, walker, Math::point(40, 0, 5)));
    QVERIFY(WalkUntilStopped(world, walker, 400) == MoveState::Unreachable);
    position = world->GetComponent<Game::Position>(walker);
    QVERIFY(position.x > 18.0f && position.x <= 20.0f);

    // a target far from any navmesh fails right away
    QVERIFY(SetMoveTarget(world, walker, Math::point(100, 0, 100)));
    QVERIFY(GetMoveState(world, walker) == MoveState::Failed);

    // stopping halfway leaves the agent where it is
    QVERIFY(SetMoveTarget(world, walker, Math::point(10, 0, 18)));
    for (int i = 0; i < 20; i++)
    {
        AgentManager::UpdateAgents(world, 0.05f);
    }
    StopMoving(world, walker);
    QVERIFY(GetMoveState(world, walker) == MoveState::Idle);
    const Math::vec3 stopped = world->GetComponent<Game::Position>(walker);
    for (int i = 0; i < 20; i++)
    {
        AgentManager::UpdateAgents(world, 0.05f);
    }
    position = world->GetComponent<Game::Position>(walker);
    QVERIFY(Math::length(position - stopped) < 0.05f);

    // moving the entity, like the editor does, takes the agent with it
    world->SetComponent<Game::Position>(walker, Game::Position(5, 0, 15));
    QVERIFY(SetMoveTarget(world, walker, Math::point(15, 0, 15)));
    QVERIFY(WalkUntilStopped(world, walker, 400) == MoveState::Arrived);
    position = world->GetComponent<Game::Position>(walker);
    QVERIFY(Near(position.x, 15.0f, 0.3f) && Near(position.z, 15.0f, 0.3f));

    // with a velocity the agent only sets it, what moves the entity by it is followed by the crowd
    Game::Entity const character = CreateAgent(world, "walker", Math::vec3(2, 0, 10), true);
    StepFrame();
    QVERIFY(SetMoveTarget(world, character, Math::point(12, 0, 10)));
    AgentManager::UpdateAgents(world, 0.05f);
    QVERIFY(world->GetComponent<Game::Velocity>(character).x > 0.0f);
    position = world->GetComponent<Game::Position>(character);
    QVERIFY(Near(position.x, 2.0f, 0.001f));
    QVERIFY(WalkUntilStopped(world, character, 400, true) == MoveState::Arrived);
    position = world->GetComponent<Game::Position>(character);
    QVERIFY(Near(position.x, 12.0f, 0.4f) && Near(position.z, 10.0f, 0.4f));
    QVERIFY(world->GetComponent<Game::Velocity>(character).x == 0.0f);

    // an agent kind without a navmesh waits for one
    Game::Entity const flyer = CreateAgent(world, "flyer", Math::vec3(5, 0, 5), false);
    StepFrame();
    QVERIFY(SetMoveTarget(world, flyer, Math::point(10, 0, 10)));
    AgentManager::UpdateAgents(world, 0.05f);
    QVERIFY(GetMoveState(world, flyer) == MoveState::NoNavMesh);

    // the crowd goes with its navmesh, the agent keeps its target and walks on once the navmesh is back
    QVERIFY(SetMoveTarget(world, walker, Math::point(5, 0, 5)));
    AgentManager::UpdateAgents(world, 0.05f);
    world->DeleteEntity(navMeshEntity);
    StepFrame();
    AgentManager::UpdateAgents(world, 0.05f);
    QVERIFY(GetMoveState(world, walker) == MoveState::NoNavMesh);
    navMeshEntity = CreateNavMeshEntity(world, "navtest:walker:nav");
    StepFrame();
    AgentManager::UpdateAgents(world, 0.0f);
    QVERIFY(GetMoveState(world, walker) == MoveState::Moving);
    QVERIFY(WalkUntilStopped(world, walker, 400) == MoveState::Arrived);

    // removing the component removes the agent
    world->RemoveComponent<NavAgent>(walker);
    StepFrame();
    QVERIFY(!SetMoveTarget(world, walker, Math::point(5, 0, 5)));
    QVERIFY(GetMoveState(world, walker) == MoveState::Idle);

    world->DeleteEntity(walker);
    world->DeleteEntity(character);
    world->DeleteEntity(flyer);
    world->DeleteEntity(navMeshEntity);
    StepFrame();
    gameplay->timeFactor = timeFactor;
#undef QVERIFY
}

//------------------------------------------------------------------------------
/**
    More agents than a crowd starts out with, on the walker navmesh. The crowd is made again for all
    of them, and every one of them walks
*/
static void
RunManyAgents(TestCase* test, Game::World* world)
{
#define QVERIFY(exp) test->Verify(exp, #exp, __FILE__, __LINE__)
    Game::TimeSource* gameplay = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    const float timeFactor = gameplay->timeFactor;
    gameplay->timeFactor = 0.0f;

    Game::Entity const navMeshEntity = CreateNavMeshEntity(world, "navtest:walker:nav");
    Util::Array<Game::Entity> agents;
    for (int z = 0; z < 10; z++)
    {
        for (int x = 0; x < 10; x++)
        {
            agents.Append(CreateAgent(world, "walker", Math::vec3(1.5f + x * 1.8f, 0.0f, 1.5f + z * 1.8f), false));
        }
    }
    StepFrame();

    // each walks to the spot mirrored on the quad, through the others
    for (Game::Entity entity : agents)
    {
        const Math::vec3 position = world->GetComponent<Game::Position>(entity);
        QVERIFY(SetMoveTarget(world, entity, Math::point(20.0f - position.x, 0.0f, 20.0f - position.z)));
    }
    for (int i = 0; i < 40; i++)
    {
        AgentManager::UpdateAgents(world, 0.05f);
    }

    int waiting = 0;
    int moving = 0;
    int walking = 0;
    for (Game::Entity entity : agents)
    {
        const MoveState state = GetMoveState(world, entity);
        waiting += state == MoveState::NoNavMesh ? 1 : 0;
        moving += state == MoveState::Moving ? 1 : 0;
        const Math::vec3 velocity = GetAgentVelocity(world, entity);
        walking += Math::length(Math::vec3(velocity.x, 0.0f, velocity.z)) > 1.0f ? 1 : 0;
    }
    QVERIFY(waiting == 0);
    QVERIFY(moving == agents.Size());
    // most are under way, the ones in the middle wait for the others to pass
    QVERIFY(walking >= agents.Size() / 2);

    // the update split around the crowd jobs, the way a frame runs it. Targets set and agents stopped while
    // the crowds step don't touch them, and take effect when the update finishes and starts again
    AgentManager::StartUpdate(world, 0.05f);
    const Math::vec3 velocityBefore = GetAgentVelocity(world, agents[0]);
    for (IndexT i = 0; i < agents.Size(); i++)
    {
        if (i % 4 == 0)
        {
            StopMoving(world, agents[i]);
        }
        else if (i % 4 == 1)
        {
            QVERIFY(SetMoveTarget(world, agents[i], Math::point(10.0f, 0.0f, 10.0f)));
        }
    }
    // a target far from the navmesh fails right away, the crowd isn't asked
    QVERIFY(SetMoveTarget(world, agents[2], Math::point(100.0f, 0.0f, 100.0f)));
    QVERIFY(GetMoveState(world, agents[2]) == MoveState::Failed);
    QVERIFY(GetMoveState(world, agents[0]) == MoveState::Idle);
    QVERIFY(GetAgentVelocity(world, agents[0]) == Math::vec3(0.0f));
    QVERIFY(GetMoveState(world, agents[1]) == MoveState::Moving);
    AgentManager::FinishUpdate(world);
    QVERIFY(Math::length(velocityBefore) > 0.0f);

    for (int i = 0; i < 20; i++)
    {
        AgentManager::StartUpdate(world, 0.05f);
        AgentManager::FinishUpdate(world);
    }
    int stoppedStill = 0;
    int retargetedMoving = 0;
    for (IndexT i = 0; i < agents.Size(); i++)
    {
        if (i % 4 == 0)
        {
            stoppedStill += GetMoveState(world, agents[i]) == MoveState::Idle && GetAgentVelocity(world, agents[i]) == Math::vec3(0.0f) ? 1 : 0;
        }
        else if (i % 4 == 1)
        {
            const MoveState state = GetMoveState(world, agents[i]);
            retargetedMoving += state == MoveState::Moving || state == MoveState::Arrived ? 1 : 0;
        }
    }
    QVERIFY(stoppedStill == agents.Size() / 4);
    QVERIFY(retargetedMoving == agents.Size() / 4);

    for (Game::Entity entity : agents)
    {
        world->DeleteEntity(entity);
    }
    world->DeleteEntity(navMeshEntity);
    StepFrame();
    gameplay->timeFactor = timeFactor;
#undef QVERIFY
}

//------------------------------------------------------------------------------
/**
    One physics frame, which is when the character controllers move by their velocity
*/
static void
StepPhysicsFrame()
{
    Game::TimeSource* physicsTime = Game::Time::GetTimeSource(TIMESOURCE_PHYSICS);
    physicsTime->timeFactor = 0.0f;
    Game::Time::AdvanceTimeSource(physicsTime, PHYSICS_RATE);
    StepFrame();
}

//------------------------------------------------------------------------------
/**
    An agent with a physics character controller, on a box the size of the arena navmesh. The agent
    only sets the velocity, the controller moves the entity on the physics frame
*/
static void
RunCharacter(TestCase* test, Game::World* world)
{
#define QVERIFY(exp) test->Verify(exp, #exp, __FILE__, __LINE__)
    Game::TimeSource* gameplay = Game::Time::GetTimeSource(TIMESOURCE_GAMEPLAY);
    const float timeFactor = gameplay->timeFactor;
    gameplay->timeFactor = 0.0f;

    // the arena navmesh is a 20x20 quad from 200 to 220, away from what the physics tests leave behind
    Game::Entity const navMeshEntity = CreateNavMeshEntity(world, "navtest:arena:nav");
    Game::Entity const ground = world->CreateEntity();
    world->SetComponent<Game::Position>(ground, Game::Position(210.0f, -0.5f, 210.0f));
    world->SetComponent<Game::Orientation>(ground, Game::Orientation(Math::quat()));
    world->AddComponent<PhysicsFeature::StaticBody>(ground);
    PhysicsFeature::BoxCollider* box = world->AddComponent<PhysicsFeature::BoxCollider>(ground);
    box->extents = Math::vec3(10.0f, 0.5f, 10.0f);
    StepPhysicsFrame();
    StepPhysicsFrame();

    // a capsule 1.8 high, its center placed so its foot is on the ground
    Physics::CharacterCreateInfo info;
    info.type = Physics::ColliderType_Capsule;
    info.capsule.radius = 0.4f;
    info.capsule.height = 1.0f;
    info.capsule.constrainClimbing = false;
    const Math::vec3 start(202.0f, 0.0f, 202.0f);
    const float centerHeight = info.capsule.radius + info.capsule.height * 0.5f + info.contactOffset;
    const Physics::CharacterId controller = Physics::CharacterContext::CreateCharacter(
        start + Math::vec3(0.0f, centerHeight, 0.0f), info, PhysicsFeature::PhysicsFeatureUnit::Instance()->SceneForWorld(world));

    Game::Entity const entity = world->CreateEntity();
    world->SetComponent<Game::Position>(entity, Game::Position(start.x, start.y, start.z));
    world->AddComponent<Game::Velocity>(entity);
    PhysicsFeature::Character character;
    character.characterId = controller;
    world->AddComponent(entity, Game::GetComponentId<PhysicsFeature::Character>(), &character);
    NavAgent agent;
    agent.agentKind = "runner";
    world->AddComponent(entity, Game::GetComponentId<NavAgent>(), &agent);
    // the adds are executed with the next frame
    StepFrame();

    // a controller only finds the ground when it moves into it, gravity does that while it stands
    auto applyGravity = [world, entity]()
    {
        Game::Velocity velocity = world->GetComponent<Game::Velocity>(entity);
        if (!world->GetComponent<PhysicsFeature::Character>(entity).isGrounded)
        {
            velocity.y -= 9.81f * PHYSICS_RATE;
        }
        else
        {
            velocity.y = Math::min(velocity.y, -0.1f);
        }
        world->SetComponent<Game::Velocity>(entity, velocity);
    };
    for (int i = 0; i < 10; i++)
    {
        applyGravity();
        StepPhysicsFrame();
    }
    QVERIFY(world->GetComponent<PhysicsFeature::Character>(entity).isGrounded);

    // the agent sets the velocity before each physics frame, gravity is the game's, like the character
    // manager expects it
    const Math::point target(218.0f, 0.0f, 210.0f);
    QVERIFY(SetMoveTarget(world, entity, target));
    float speed = 0.0f;
    for (int i = 0; i < 800 && GetMoveState(world, entity) == MoveState::Moving; i++)
    {
        AgentManager::UpdateAgents(world, PHYSICS_RATE);
        const Game::Velocity velocity = world->GetComponent<Game::Velocity>(entity);
        speed = Math::max(speed, Math::length(Math::vec3(velocity.x, 0.0f, velocity.z)));
        applyGravity();
        StepPhysicsFrame();
    }
    QVERIFY(GetMoveState(world, entity) == MoveState::Arrived);
    QVERIFY(speed > 3.0f && speed <= 3.6f);
    const Math::vec3 position = world->GetComponent<Game::Position>(entity);
    QVERIFY(Near(position.x, target.x, 0.4f) && Near(position.z, target.z, 0.4f));
    // the controller kept the foot on the ground all the way
    QVERIFY(Near(position.y, 0.0f, 0.2f));
    const Math::vec3 foot = Physics::CharacterContext::GetCharacterFootPosition(controller);
    QVERIFY(Near(foot.x, position.x, 0.01f) && Near(foot.z, position.z, 0.01f));

    // cleaning up a world, which stopping the game in the editor does, releases the controllers of its
    // characters and clears their ids, so the decay which follows doesn't release them again
    PhysicsFeature::CharacterManager::Instance()->OnCleanup(world);
    QVERIFY(world->GetComponent<PhysicsFeature::Character>(entity).characterId.id == Physics::CharacterId().id);

    world->DeleteEntity(entity);
    world->DeleteEntity(ground);
    world->DeleteEntity(navMeshEntity);
    StepPhysicsFrame();
    gameplay->timeFactor = timeFactor;
#undef QVERIFY
}

//------------------------------------------------------------------------------
/**
*/
void
NavigationManagerTest::Run()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    n_assert(world != nullptr);
    VERIFY(Navigation::navMeshCache != nullptr);

    // the navmeshes are baked into a temp folder, not into the exports of the project
    IO::AssignRegistry* assigns = IO::AssignRegistry::Instance();
    const Util::String navAssign = assigns->GetAssign("nav");
    IO::CreateDirectory(IO::URI("temp:navigationtest"));
    assigns->SetAssign(IO::Assign("nav", "temp:navigationtest"));

    const IO::Path human = IO::Path::Parse("navtest:human:nav");
    const IO::Path giant = IO::Path::Parse("navtest:giant:nav");
    const IO::Path walker = IO::Path::Parse("navtest:walker:nav");
    const IO::Path arena = IO::Path::Parse("navtest:arena:nav");
    const bool bakedHuman = BakeQuad(human, "human");
    const bool bakedGiant = BakeQuad(giant, "giant");
    const bool bakedWalker = BakeQuad(walker, "walker", true);
    const bool bakedArena = BakeQuad(arena, "runner", false, Math::vec3(200.0f, 0.0f, 200.0f));
    VERIFY(bakedHuman && bakedGiant && bakedWalker && bakedArena);
    if (!bakedHuman || !bakedGiant || !bakedWalker || !bakedArena)
    {
        assigns->SetAssign(IO::Assign("nav", navAssign));
        return;
    }
    const SizeT before = ActiveCount(world);

    // a NavMesh component loads its navmesh, which is found by the entity and by the agent kind
    Game::Entity const first = CreateNavMeshEntity(world, "navtest:human:nav");
    StepFrame();
    const Navigation::NavMeshId humanMesh = GetNavMesh(world, first);
    VERIFY(humanMesh != Navigation::InvalidNavMeshId);
    VERIFY(Navigation::navMeshCache->GetDetourMesh(humanMesh) != nullptr);
    VERIFY(Navigation::navMeshCache->GetQuery(humanMesh) != nullptr);
    VERIFY(FindNavMesh(world, "human") == humanMesh);
    VERIFY(FindNavMesh(world, "giant") == Navigation::InvalidNavMeshId);
    VERIFY(ActiveCount(world) == before + 1);

    // two entities with the same navmesh share it, removing one keeps it loaded for the other
    Game::Entity const second = CreateNavMeshEntity(world, "navtest:human:nav");
    StepFrame();
    VERIFY(GetNavMesh(world, second) == humanMesh);
    VERIFY(ActiveCount(world) == before + 2);
    world->RemoveComponent<NavMesh>(first);
    StepFrame();
    VERIFY(GetNavMesh(world, first) == Navigation::InvalidNavMeshId);
    VERIFY(GetNavMesh(world, second) == humanMesh);
    VERIFY(Navigation::navMeshCache->GetDetourMesh(humanMesh) != nullptr);
    VERIFY(FindNavMesh(world, "human") == humanMesh);
    VERIFY(ActiveCount(world) == before + 1);

    // a new value, the way the editor sets one, releases the old navmesh and loads the new one
    {
        NavMesh value;
        value.resource = "navtest:giant:nav";
        world->ReinitializeComponent(second, Game::GetComponentId<NavMesh>(), &value, sizeof(NavMesh));
        VERIFY(FindNavMesh(world, "human") == Navigation::InvalidNavMeshId);
        const Navigation::NavMeshId giantMesh = GetNavMesh(world, second);
        VERIFY(giantMesh != Navigation::InvalidNavMeshId);
        VERIFY(FindNavMesh(world, "giant") == giantMesh);
        VERIFY(Navigation::navMeshCache->GetAgent(giantMesh).agent_radius == 0.5f);
        VERIFY(ActiveCount(world) == before + 1);
    }

    // an empty resource, which a component has right after it is created in the editor, loads nothing,
    // neither does a URN of another type
    Game::Entity const empty = CreateNavMeshEntity(world, "");
    Game::Entity const wrongType = CreateNavMeshEntity(world, "navtest:human:phys");
    StepFrame();
    VERIFY(GetNavMesh(world, empty) == Navigation::InvalidNavMeshId);
    VERIFY(GetNavMesh(world, wrongType) == Navigation::InvalidNavMeshId);
    VERIFY(ActiveCount(world) == before + 1);

    // a navmesh which was never baked is registered but not loaded, it is never found by the agent kind
    Game::Entity const missing = CreateNavMeshEntity(world, "navtest:missing:nav");
    StepFrame();
    VERIFY(ActiveCount(world) == before + 2);
    VERIFY(GetNavMesh(world, missing) == Navigation::InvalidNavMeshId);
    VERIFY(FindNavMesh(world, "giant") == GetNavMesh(world, second));

    // deleting the entities releases their navmeshes
    world->DeleteEntity(second);
    world->DeleteEntity(missing);
    world->DeleteEntity(empty);
    world->DeleteEntity(wrongType);
    world->DeleteEntity(first);
    StepFrame();
    VERIFY(ActiveCount(world) == before);
    VERIFY(FindNavMesh(world, "giant") == Navigation::InvalidNavMeshId);

    // the queries run on a navmesh found by its agent kind
    {
        Game::Entity const entity = CreateNavMeshEntity(world, "navtest:walker:nav");
        StepFrame();
        const Navigation::NavMeshId navMesh = FindNavMesh(world, "walker");
        VERIFY(navMesh != Navigation::InvalidNavMeshId);
        if (navMesh != Navigation::InvalidNavMeshId)
        {
            RunQueries(this, navMesh);
        }
        world->DeleteEntity(entity);
        StepFrame();
    }

    RunAgents(this, world);
    RunManyAgents(this, world);
    RunCharacter(this, world);

    IO::DeleteFile(human.ExportURI());
    IO::DeleteFile(giant.ExportURI());
    IO::DeleteFile(walker.ExportURI());
    IO::DeleteFile(arena.ExportURI());
    assigns->SetAssign(IO::Assign("nav", navAssign));
}

} // namespace Test
