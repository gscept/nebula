//------------------------------------------------------------------------------
//  graphicsproperty.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsproperty.h"
#include "basegamefeature/managers/categorymanager.h"
#include "basegamefeature/properties/transformableproperty.h"
#include "debug/debugtimer.h"
#include "models/modelcontext.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"

namespace GraphicsFeature
{

_declare_static_timer(GraphicsComponentOnEndFrame);

__ImplementClass(GraphicsFeature::GraphicsProperty, 'TRPR', Game::Property);

//------------------------------------------------------------------------------
/**
*/
GraphicsProperty::GraphicsProperty()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
GraphicsProperty::~GraphicsProperty()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsProperty::Init()
{
	this->data = {
		Game::CreatePropertyState<State>(this->category);
		Game::GetPropertyData<Attr::WorldTransform>(this->category);
	};
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsProperty::OnActivate(Game::InstanceId instance)
{
	auto gfxEntity = Graphics::CreateEntity();
	this->data.state[instance.id].gfxEntity = gfxEntity;
	Models::ModelContext::RegisterEntity(gfxEntity);
	Models::ModelContext::Setup(gfxEntity, component->Get<Attr::ModelResource>(instance), "NONE");
	auto transform = this->data.worldTransforms[instance.id];
	Models::ModelContext::SetTransform(gfxEntity, transform);
	Visibility::ObservableContext::RegisterEntity(gfxEntity);
	Visibility::ObservableContext::Setup(gfxEntity, Visibility::VisibilityEntityType::Model);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsProperty::OnDeactivate(Game::InstanceId instance)
{
	Graphics::GraphicsEntityId gfxEntity = this->data.state[instance].gfxEntity;
	Visibility::ObservableContext::DeregisterEntity(gfxEntity);
	Models::ModelContext::DeregisterEntity(gfxEntity);
	Graphics::DestroyEntity(gfxEntity);
    this->data.state[instance].gfxEntity = Graphics::GraphicsEntityId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsFeature::OnBeginFrame()
{
	_start_timer(GraphicsComponentOnEndFrame);

    for (int i = 0; i < this->data.state.Size(); ++i)
    {
        Graphics::GraphicsEntityId gfxEntity = this->data.state[i];
        if (gfxEntity != Graphics::GraphicsEntityId::Invalid())
        {
            Models::ModelContext::SetTransform(gfxEntity, this->data.worldTransform[i]);
        }
    }

    _stop_timer(GraphicsComponentOnEndFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsProperty::SetupExternalAttributes()
{
	SetupAttr(Attr::WorldTransform::Id());
}

} // namespace GraphicsFeature
