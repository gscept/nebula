//------------------------------------------------------------------------------
//  game/basegamefeature.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "core/factory.h"
#include "game/gameserver.h"
#include "game/gameserver.h"
#include "io/ioserver.h"
#include "io/console.h"
#include "basegamefeature/components/transformcomponent.h"

namespace BaseGameFeature
{
__ImplementClass(BaseGameFeature::BaseGameFeatureUnit, 'GAGF' , Game::FeatureUnit);
__ImplementSingleton(BaseGameFeatureUnit);

using namespace App;
using namespace Game;

//------------------------------------------------------------------------------
/**
*/
BaseGameFeatureUnit::BaseGameFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
BaseGameFeatureUnit::~BaseGameFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();
    
	this->entityManager = EntityManager::Create();
	this->componentManager = ComponentManager::Create();
	this->AttachManager(this->entityManager.upcast<Game::Manager>());
	this->AttachManager(this->componentManager.upcast<Game::Manager>());

	Game::TransformComponent::Create();
	Game::TagComponent::Create();

	this->loaderServer = LoaderServer::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnDeactivate()
{
	this->componentManager->ClearAll();

    this->RemoveManager(this->entityManager.upcast<Game::Manager>());
	this->RemoveManager(this->componentManager.upcast<Game::Manager>());

    this->entityManager = nullptr;
	this->componentManager = nullptr;

	Game::TransformComponent::Discard();
	Game::TagComponent::Discard();

	this->loaderServer->Release();
	this->loaderServer = nullptr;

    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnRenderDebug()
{
    // render debug for all entities and its properties
    FeatureUnit::OnRenderDebug();
}

//------------------------------------------------------------------------------
/**
    Setup a new, empty world.
*/
void
BaseGameFeatureUnit::SetupEmptyWorld()
{
	Game::GameServer::Instance()->NotifyBeforeLoad();
}

//------------------------------------------------------------------------------
/**
    Cleanup the game world. This should undo the stuff in SetupWorld().
    Override this method in a subclass if your app needs different 
    behaviour.
*/
void
BaseGameFeatureUnit::CleanupWorld()
{
	// cleanup

	n_assert2(false, "Not implemented!");
	// this->entityManager->Cleanup();
	// this->componentManager->Cleanup();

	Game::GameServer::Instance()->NotifyBeforeCleanup();            
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnEndFrame()
{
    FeatureUnit::OnEndFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::OnFrame()
{    
    FeatureUnit::OnFrame();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseGameFeatureUnit::WriteAdditionalMetadata(Ptr<IO::JsonWriter> const& writer) const
{
	writer->BeginArray("components");
	SizeT numComponents = Game::ComponentManager::Instance()->GetNumComponents();
	for (IndexT i = 0; i < numComponents; ++i)
	{
		auto const& component = Game::ComponentManager::Instance()->GetComponentAtIndex(i);
		Util::String componentName = component->GetName().AsString();

		writer->BeginObject();

		writer->Add(componentName.AsCharPtr(), "name");
		Util::FourCC fourcc = component->GetIdentifier();
		writer->Add(fourcc.AsString(), "fourcc");
		// writer->Add(component->GetEvents())
		
		
		writer->BeginArray("events");
		auto events = component->SubscribedEvents();
		{
			for (IndexT e = 0; e < Game::ComponentEvent::NumEvents; ++e)
			{
				if (events.IsSet(e))
				{
					writer->Add(Game::ComponentEventNames[e]);
				}
			}
		}
		writer->End();

		writer->BeginArray("attributes");

		for (auto const& attr : component->GetAttributes())
		{
			writer->BeginObject();
			writer->Add(attr.GetName(), "name");
			writer->Add(attr.GetFourCC().AsString(), "fourcc");

			if (attr.GetValueType() == Attr::ValueType::EntityType)
			{
				/// @todo	this should be not be hardcoded.
				writer->Add("entity", "type");
			}
			else
			{
				writer->Add(Util::Variant::TypeToString((Util::Variant::Type)attr.GetValueType()), "type");
			}

			Util::String accessMode;
			if (attr.GetAccessMode() == Attr::AccessMode::ReadOnly)
			{
				accessMode = "R";
			}
			else
			{
				accessMode = "RW";
			}

			writer->Add(accessMode, "access");
			
			const char* key = "default";

			switch (attr.GetValueType())
			{
			//case Attr::ValueType::IntType:			writer->Add(attr.GetDefaultValue().GetInt(), key);			break;
			//case Attr::ValueType::Int64Type:		writer->Add(attr.GetDefaultValue().GetInt64(), key);		break;
			//case Attr::ValueType::UIntType:			writer->Add(attr.GetDefaultValue().GetUInt(), key);			break;
			//case Attr::ValueType::UInt64Type:		writer->Add(attr.GetDefaultValue().GetUInt64(), key);		break;
			//case Attr::ValueType::FloatType:		writer->Add(attr.GetDefaultValue().GetFloat(), key);		break;
			//case Attr::ValueType::DoubleType:		writer->Add(attr.GetDefaultValue().GetDouble(), key);		break;
			//case Attr::ValueType::BoolType:			writer->Add(attr.GetDefaultValue().GetBool(), key);			break;
			//case Attr::ValueType::ShortType:		writer->Add(attr.GetDefaultValue().GetShort(), key);		break;
			//case Attr::ValueType::UShortType:		writer->Add(attr.GetDefaultValue().GetUShort(), key);		break;
			//case Attr::ValueType::ByteType:			writer->Add(attr.GetDefaultValue().GetByte(), key);			break;
			//case Attr::ValueType::GuidType:			writer->Add(attr.GetDefaultValue().GetGuid(), key);			break;
			//case Attr::ValueType::Float2Type:		writer->Add(attr.GetDefaultValue().GetFloat2(), key);		break;
			//case Attr::ValueType::Float4Type:		writer->Add(attr.GetDefaultValue().GetFloat4(), key);		break;
			//case Attr::ValueType::QuaternionType:	writer->Add(attr.GetDefaultValue().GetQuaternion(), key);	break;
			//case Attr::ValueType::Matrix44Type:		writer->Add(attr.GetDefaultValue().GetMatrix44(), key);		break;
			//case Attr::ValueType::EntityType:		writer->Add(attr.GetDefaultValue().GetUInt(), key);			break;
			default:
				n_warning("Attribute type %i not fully supported. Check basegamefeatureunit and add it to the list!", attr.GetValueType());
				writer->Add(attr.GetDefaultValue().ToString(), "default");
				break;
			}
			writer->End();
		}

		writer->End();
		writer->End();
	}
	writer->End();
}

} // namespace Game
