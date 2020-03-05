//------------------------------------------------------------------------------
//  factorymanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "factorymanager.h"
#include "entitymanager.h"
#include "categorymanager.h"
#include "io/jsonreader.h"
#include "game/property.h"
#include "io/ioserver.h"

namespace Game
{

__ImplementClass(Game::FactoryManager, 'MFAM', Game::Manager);
__ImplementSingleton(FactoryManager)

Util::String FactoryManager::blueprintFilename("blueprints.json");
Util::String FactoryManager::blueprintFolder("data:tables/");

//------------------------------------------------------------------------------
/**
*/
FactoryManager::FactoryManager()
{
	__ConstructSingleton;
	if (!this->ParseBluePrints())
	{
		n_error("Managers::FactoryManager: Error parsing data:tables/blueprints.json!");
	}
}

//------------------------------------------------------------------------------
/**
*/
FactoryManager::~FactoryManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
FactoryManager::OnActivate()
{
	this->SetupCategories();
	this->SetupAttributes();
	Game::Manager::OnActivate();
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
FactoryManager::CreateEntityByCategory(Util::StringAtom const categoryName) const
{
	Entity entity = EntityManager::Instance()->CreateEntity();
	Ptr<CategoryManager> cm = CategoryManager::Instance();
	CategoryId cid = cm->GetCategoryId(categoryName);

	InstanceId instance = cm->AllocateInstance(entity, cid);

	Category const& category = cm->GetCategory(cid);

	for (auto const& prop : category.properties)
	{
		prop->OnActivate(instance);
	}

	return entity;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
FactoryManager::CreateEntity(EntityCreateInfo const& info) const
{
	Entity entity = EntityManager::Instance()->CreateEntity();
	Ptr<CategoryManager> cm = CategoryManager::Instance();
	CategoryId cid = cm->GetCategoryId(info.categoryName);

	InstanceId instance = cm->AllocateInstance(entity, cid);

	Category const& category = cm->GetCategory(cid);

	// Set attributes before activating
	// TODO: We should probably create a Game namespace abstraction for setting an attribute value by AttributeId and AttributeValue
	Ptr<Db::Database> db = EntityManager::Instance()->GetWorldDatabase();
	SizeT numAttrs = info.attributes.Size();
	for (IndexT i = 0; i < numAttrs; ++i)
	{
		//Db::Table const& table = db->GetTable(category.instanceTable);
		Db::ColumnId columnId = db->GetColumnId(category.instanceTable, info.attributes[i].Key());
		n_assert(columnId != Db::ColumnId::Invalid());
		db->Set(category.instanceTable, columnId, instance.id, info.attributes[i].Value());
	}

	for (auto const& prop : category.properties)
	{
		prop->OnActivate(instance);
	}

	return entity;
}

//------------------------------------------------------------------------------
/**
	Create a property by its type name.
*/
Ptr<Property>
FactoryManager::CreateProperty(const Util::String& type) const
{
	Game::Property* result = (Game::Property*) Core::Factory::Instance()->Create(type);
	n_assert(result != 0);
	return result;
}

//------------------------------------------------------------------------------
/**
	This method parses the file data:tables/blueprints.json into
	the bluePrints array.
*/
bool
FactoryManager::ParseBluePrints()
{
	// it is not an error here if blueprints.json doesn't exist
	Util::String blueprintsPath = FactoryManager::blueprintFolder;
	blueprintsPath.Append(FactoryManager::blueprintFilename);

	if (IO::IoServer::Instance()->FileExists(blueprintsPath))
	{
		Ptr<IO::JsonReader> jsonReader = IO::JsonReader::Create();
		jsonReader->SetStream(IO::IoServer::Instance()->CreateStream(blueprintsPath));
		if (jsonReader->Open())
		{
			// make sure it's a BluePrints file
			if (!jsonReader->SetToNode("blueprints"))
			{
				n_error("FactoryManager::ParseBluePrints(): not a valid blueprints file!");
				return false;
			}
			if (jsonReader->SetToFirstChild()) do
			{
				BluePrint bluePrint;
				bluePrint.name = jsonReader->GetCurrentNodeName();

				if (jsonReader->SetToFirstChild("properties"))
				{
					jsonReader->SetToFirstChild();
					do
					{
						PropertyEntry newProp;
						newProp.propertyName = jsonReader->GetString();

						bluePrint.properties.Append(newProp);
					} while (jsonReader->SetToNextChild());

					jsonReader->SetToParent();
				}

				this->bluePrints.Append(bluePrint);

				//jsonReader->SetToParent();
			} while (jsonReader->SetToNextChild());

			jsonReader->Close();
			return true;
		}
		else
		{
			n_error("Managers::FactoryManager::ParseBluePrints(): could not open '%s'!", blueprintsPath.AsCharPtr());
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
FactoryManager::SetBlueprintsFilename(const Util::String& name, const Util::String& folder)
{
	n_assert(name.IsValid());
	blueprintFilename = name;
	blueprintFolder = folder;
}

//------------------------------------------------------------------------------
/**
	Create the properties of every category and call SetupDefaultAttributes on it.
*/
void
FactoryManager::SetupAttributes()
{
	// create a instance of every property and call SetupDefaultAttributes()
	IndexT idxBluePrint;
	for (idxBluePrint = 0; idxBluePrint < this->bluePrints.Size(); idxBluePrint++)
	{
		const BluePrint& bluePrint = this->bluePrints[idxBluePrint];

		// category for blueprint type not found
		if (!CategoryManager::Instance()->HasCategory(bluePrint.name))
		{
			n_printf("Obsolete Category '%s' in blueprints.json", bluePrint.name.AsCharPtr());
			continue;
		}

		// begin add category attrs
		CategoryManager::Instance()->BeginAddCategoryAttrs(bluePrint.name);

		const Util::Array<PropertyEntry>& catProperties = bluePrint.properties;
		IndexT idxCatProperty;
		for (idxCatProperty = 0; idxCatProperty < catProperties.Size(); idxCatProperty++)
		{
			const Util::String& propertyName = catProperties[idxCatProperty].propertyName;
			if (Core::Factory::Instance()->ClassExists(propertyName))
			{
				Ptr<Game::Property> newProperty = this->CreateProperty(propertyName);
				CategoryManager::Instance()->AddProperty(newProperty);
			}
			else
			{
				n_warning("Blueprint '%s' contains invalid property named '%s'!\n", bluePrint.name.AsCharPtr(), propertyName.AsCharPtr());
			}
		}
		CategoryManager::Instance()->EndAddCategoryAttrs();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FactoryManager::SetupCategories()
{

	// create a instance of every property and call SetupDefaultAttributes()
	IndexT idxBluePrint;
	for (idxBluePrint = 0; idxBluePrint < this->bluePrints.Size(); idxBluePrint++)
	{
		const BluePrint& bluePrint = this->bluePrints[idxBluePrint];

		if (CategoryManager::Instance()->HasCategory(bluePrint.name))
		{
			n_warning("Duplicate blueprint named '%s' found in blueprints.json\n", bluePrint.name.AsCharPtr());
			continue;
		}
		CategoryCreateInfo info;
		info.name = bluePrint.name;
		// Note that we don't setup any attributes for this category yet!

		CategoryManager::Instance()->AddCategory(info);
	}
}


} // namespace Game


