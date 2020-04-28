//------------------------------------------------------------------------------
//  jsonentityloader.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "jsonentityloader.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/messages/basegameprotocol.h"

namespace BaseGameFeature
{

__ImplementClass(BaseGameFeature::JsonEntityLoader, 'jsel', BaseGameFeature::EntityLoaderBase);

bool JsonEntityLoader::insideLoading = false;

//------------------------------------------------------------------------------
/**
*/
JsonEntityLoader::JsonEntityLoader()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JsonEntityLoader::~JsonEntityLoader()
{
    // empty
}

struct Listener
{
	Game::ComponentInterface* component;
	uint instance;
};

//------------------------------------------------------------------------------
/**
*/
bool
JsonEntityLoader::Load(const Util::String& file)
{    
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
	reader->SetStream(IO::IoServer::Instance()->CreateStream(file));
	if (reader->Open())
    {
		reader->SetToFirstChild();
		do
		{
			Game::Entity entity = Game::EntityManager::Instance()->NewEntity();
			Math::mat4 transform = reader->GetOptMat4("transform", Math::mat4());

			// We need to save each component and enitity start index so that we can call activate after
			// all components has been loaded
			Util::Array<Listener> activateListeners;

			if (reader->HasNode("components"))
			{
				reader->SetToFirstChild("components");
				reader->SetToFirstChild();
				do
				{
					Util::String name = reader->GetCurrentNodeName();
					Game::ComponentInterface* component = Game::ComponentManager::Instance()->GetComponentByName(name);
					if (component == nullptr)
					{
						// Could not find component by name in registry
						continue;
					}

					component->Allocate(1);
					Game::InstanceId instance = component->NumRegistered() - 1;

					component->SetOwner(instance, entity);


					auto const& attrids = component->GetAttributes();
					for (SizeT i = 0; i < attrids.Size(); ++i)
					{
                        Attr::AttrId attr = attrids[i];

						auto attrName = attr.GetName();
						Util::Variant value;
						value.SetType(attr.GetDefaultValue().GetType());
						// Retrieve the value. Reader parses the type that is provided.
						if (reader->GetOpt(value, attrName.AsCharPtr(), attr.GetDefaultValue()))
						{
							component->SetAttributeValue(instance, attr.GetFourCC(), value);
						}
					}

					if (component->SubscribedEvents().IsSet(Game::ComponentEvent::OnLoad) && component->functions.OnLoad != nullptr)
					{
						component->functions.OnLoad(instance);
					}

					if (component->SubscribedEvents().IsSet(Game::ComponentEvent::OnActivate) && component->functions.OnActivate != nullptr)
					{
						// Add to list to that we can activate all instances in this component later.
						Listener listener;
						listener.component = component;
						listener.instance = instance;
						activateListeners.Append(listener);
					}

				} while (reader->SetToNextChild());
				
				reader->SetToParent();
			}

			for (auto& listener : activateListeners)
			{
				listener.component->functions.OnActivate(listener.instance);
			}

			Msg::SetLocalTransform::Send(entity, transform);

		} while (reader->SetToNextChild());
        reader->Close();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool 
JsonEntityLoader::IsLoading()
{
    return insideLoading;
}

} // namespace BaseGameFeature


