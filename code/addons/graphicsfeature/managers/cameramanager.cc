//------------------------------------------------------------------------------
//  cameramanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "cameramanager.h"
#include "graphics/graphicsentity.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"
#include "visibility/visibilitycontext.h"
#include "game/gameserver.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "graphics/view.h"

namespace GraphicsFeature
{

__ImplementSingleton(CameraManager)

//------------------------------------------------------------------------------
/**
*/
CameraManager::CameraManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
CameraManager::~CameraManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ViewHandle
CameraManager::RegisterView(Ptr<Graphics::View> const& view)
{
    n_assert(CameraManager::HasInstance());
    Ids::Id32 id;
    Singleton->viewHandlePool.Allocate(id);
    ViewData data;
    data.gid = Graphics::CreateEntity();
    data.view = view;
    if (Singleton->viewHandleMap.Size() <= Ids::Index(id))
        Singleton->viewHandleMap.Append(data);
    else
        Singleton->viewHandleMap[Ids::Index(id)] = data;
    
    if (view->GetCamera() != Graphics::GraphicsEntityId::Invalid())
        n_warning("WARNING: View already has a camera entity assigned which is being overridden!\n");

    Graphics::CameraContext::RegisterEntity(data.gid);
    Graphics::CameraContext::SetLODCamera(data.gid);
    Visibility::ObserverContext::RegisterEntity(data.gid);
    Visibility::ObserverContext::Setup(data.gid, Visibility::VisibilityEntityType::Camera);
    view->SetCamera(data.gid);

    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateCameraSettings(Graphics::GraphicsEntityId gid, Camera& settings, Camera const& change)
{
    // check for changes
    bool changed = false;
    changed |= settings.fieldOfView != change.fieldOfView;
    changed |= settings.orthographicWidth != change.orthographicWidth;
    changed |= settings.projectionMode != change.projectionMode;
    changed |= settings.zFar != change.zFar;
    changed |= settings.zNear != change.zNear;
    changed |= settings.aspectRatio != change.aspectRatio;

    if (changed)
    {
        if (change.projectionMode == ProjectionMode::PERSPECTIVE)
            Graphics::CameraContext::SetupProjectionFov(gid, change.aspectRatio, Math::deg2rad(change.fieldOfView), change.zNear, change.zFar);
        else
            Graphics::CameraContext::SetupOrthographic(gid, change.orthographicWidth, change.orthographicWidth * change.aspectRatio, change.zNear, change.zFar);
        settings = change;
    }
    else
    {
        settings.localTransform = change.localTransform;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CameraManager::InitUpdateCameraProcessor()
{
    { // Setup processor that handles both worldtransform and camera (heirarchy)
        Game::FilterCreateInfo filterInfo;
        filterInfo.inclusive[0] = Game::GetComponentId("Camera"_atm);
        filterInfo.access   [0] = Game::AccessMode::READ;
        filterInfo.inclusive[1] = Game::GetComponentId("WorldTransform"_atm);
        filterInfo.access   [1] = Game::AccessMode::READ;
        filterInfo.numInclusive = 2;

        Game::Filter filter = Game::CreateFilter(filterInfo);

        Game::ProcessorCreateInfo processorInfo;
        processorInfo.async = false;
        processorInfo.filter = filter;
        processorInfo.name = "CameraManager.UpdateCameraWorldTransformed"_atm;
        processorInfo.OnBeginFrame = [](Game::World*, Game::Dataset data)
        {
            for (int v = 0; v < data.numViews; v++)
            {
                Game::Dataset::EntityTableView const& view = data.views[v];
                Camera const* const cameras = (Camera*)view.buffers[0];
                Math::mat4 const* const transforms = (Math::mat4*)view.buffers[1];

                for (IndexT i = 0; i < view.numInstances; ++i)
                {
                    Camera const& camera = cameras[i];
                    Math::mat4 const& parentTransform = transforms[i];

                    if (IsViewHandleValid(camera.viewHandle))
                    {
                        Graphics::GraphicsEntityId gid = Singleton->viewHandleMap[Ids::Index(camera.viewHandle)].gid;
                        Camera& settings = Singleton->viewHandleMap[Ids::Index(camera.viewHandle)].currentSettings;
                        UpdateCameraSettings(gid, settings, camera);
                        Graphics::CameraContext::SetView(gid, parentTransform * settings.localTransform);
                    }
                }
            }
        };

        Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
    }
    { // Setup processor that handles just a regular old camera
        Game::FilterCreateInfo filterInfo;
        filterInfo.inclusive[0] = Game::GetComponentId("Camera"_atm);
        filterInfo.access   [0] = Game::AccessMode::READ;
        filterInfo.numInclusive = 1;

        filterInfo.exclusive[0] = Game::GetComponentId("WorldTransform"_atm);
        filterInfo.numExclusive = 1;

        Game::Filter filter = Game::CreateFilter(filterInfo);

        Game::ProcessorCreateInfo processorInfo;
        processorInfo.async = false;
        processorInfo.filter = filter;
        processorInfo.name = "CameraManager.UpdateCamera"_atm;
        processorInfo.OnBeginFrame = [](Game::World*, Game::Dataset data)
        {
            for (int v = 0; v < data.numViews; v++)
            {
                Game::Dataset::EntityTableView const& view = data.views[v];
                Camera const* const cameras = (Camera*)view.buffers[0];
                
                for (IndexT i = 0; i < view.numInstances; ++i)
                {
                    Camera const& camera = cameras[i];
                    
                    if (IsViewHandleValid(camera.viewHandle))
                    {
                        Graphics::GraphicsEntityId gid = Singleton->viewHandleMap[Ids::Index(camera.viewHandle)].gid;
                        Camera& settings = Singleton->viewHandleMap[Ids::Index(camera.viewHandle)].currentSettings;
                        UpdateCameraSettings(gid, settings, camera);
                        Graphics::CameraContext::SetView(gid, settings.localTransform);
                    }
                }
            }
        };

        Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
CameraManager::Create()
{
    n_assert(!CameraManager::HasInstance());
    CameraManager::Singleton = n_new(CameraManager);
   
    Singleton->InitUpdateCameraProcessor();

    Game::ManagerAPI api;
    api.OnDeactivate = &Destroy;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
CameraManager::Destroy()
{
    n_assert(CameraManager::HasInstance());
    n_delete(CameraManager::Singleton);
    CameraManager::Singleton = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
CameraManager::GetProjection(ViewHandle handle)
{
    n_assert(CameraManager::HasInstance());
    auto gid = Singleton->viewHandleMap[Ids::Index(handle)].gid;
    return Graphics::CameraContext::GetProjection(gid);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
CameraManager::GetLocalTransform(ViewHandle handle)
{
    n_assert(CameraManager::HasInstance());
    auto gid = Singleton->viewHandleMap[Ids::Index(handle)].gid;
    return Graphics::CameraContext::GetTransform(gid);
}

} // namespace Game
