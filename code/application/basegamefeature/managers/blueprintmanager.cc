//------------------------------------------------------------------------------
//  blueprintmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "blueprintmanager.h"
#include "entitymanager.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"

namespace Game
{

__ImplementSingleton(BlueprintManager)

Util::String BlueprintManager::blueprintFilename("blueprints.json");
Util::String BlueprintManager::blueprintFolder("data:tables/");

//------------------------------------------------------------------------------
/**
*/
ManagerAPI
BlueprintManager::Create()
{
    n_assert(!BlueprintManager::HasInstance());
    Singleton = n_new(BlueprintManager);

    ManagerAPI api;
    api.OnActivate = &BlueprintManager::OnActivate;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
BlueprintManager::Destroy()
{
    n_assert(BlueprintManager::HasInstance());
    n_delete(Singleton);
}

//------------------------------------------------------------------------------
/**
*/
BlueprintManager::BlueprintManager()
{
    if (!this->ParseBlueprints())
    {
        n_error("Managers::BlueprintManager: Error parsing data:tables/blueprints.json!");
    }
}

//------------------------------------------------------------------------------
/**
*/
BlueprintManager::~BlueprintManager()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
BlueprintManager::OnActivate()
{
	Singleton->SetupCategories();
}

//------------------------------------------------------------------------------
/**
	This method parses the file data:tables/blueprints.json into
	the blueprints array.
*/
bool
BlueprintManager::ParseBlueprints()
{
	// it is not an error here if blueprints.json doesn't exist
	Util::String blueprintsPath = BlueprintManager::blueprintFolder;
	blueprintsPath.Append(BlueprintManager::blueprintFilename);

	if (IO::IoServer::Instance()->FileExists(blueprintsPath))
	{
		Ptr<IO::JsonReader> jsonReader = IO::JsonReader::Create();
		jsonReader->SetStream(IO::IoServer::Instance()->CreateStream(blueprintsPath));
		if (jsonReader->Open())
		{
			// make sure it's a BluePrints file
			if (!jsonReader->SetToNode("blueprints"))
			{
				n_error("BlueprintManager::ParseBlueprints(): not a valid blueprints file!");
				return false;
			}
			if (jsonReader->SetToFirstChild()) do
			{
				Blueprint bluePrint;
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

				this->blueprints.Append(bluePrint);

				//jsonReader->SetToParent();
			} while (jsonReader->SetToNextChild());

			jsonReader->Close();
			return true;
		}
		else
		{
			n_error("Managers::BlueprintManager::ParseBlueprints(): could not open '%s'!", blueprintsPath.AsCharPtr());
			return false;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void
BlueprintManager::SetBlueprintsFilename(const Util::String& name, const Util::String& folder)
{
	n_assert(name.IsValid());
	blueprintFilename = name;
	blueprintFolder = folder;
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
BlueprintManager::Instantiate(BlueprintId blueprint)
{
	EntityManager::State const& emState = EntityManager::Instance()->state;
	CategoryId cid = Singleton->blueprints[blueprint.id].categoryId;
	Category& cat = emState.categoryArray[cid.id];
	InstanceId instance = emState.worldDatabase->AllocateRow(cat.instanceTable);
	return { cid, instance };
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
BlueprintManager::Instantiate(BlueprintId blueprint, TemplateId templateId)
{
	EntityManager::State const& emState = EntityManager::Instance()->state;
	Ptr<MemDb::Database> const& db = emState.worldDatabase;
	CategoryId cid = Singleton->blueprints[blueprint.id].categoryId;
	Category& cat = emState.categoryArray[cid.id];
	InstanceId instance = db->DuplicateInstance(Singleton->blueprints[blueprint.id].tableId, templateId.id, cat.instanceTable);
	return { cid, instance };
}

//------------------------------------------------------------------------------
/**
*/
void
BlueprintManager::SetupCategories()
{

	// create a instance of every property and call SetupDefaultAttributes()
	IndexT idxBluePrint;
	bool failed = false;
	for (idxBluePrint = 0; idxBluePrint < this->blueprints.Size(); idxBluePrint++)
	{
		Blueprint& blueprint = this->blueprints[idxBluePrint];

		if (this->blueprintMap.Contains(blueprint.name))
		{
			n_warning("Duplicate blueprint named '%s' found in blueprints.json\n", blueprint.name.Value());
			continue;
		}
		MemDb::TableCreateInfo info;
		info.name = blueprint.name.AsString();

		const SizeT numCols = blueprint.properties.Size();
		info.columns.SetSize(numCols);

		for (int i = 0; i < numCols; i++)
		{
			auto descriptor = MemDb::TypeRegistry::GetDescriptor(blueprint.properties[i].propertyName);
			if (descriptor != PropertyId::Invalid())
			{
				info.columns[i] = descriptor;
			}
			else
			{
				n_printf("Error: Unrecognized property '%s' in blueprint '%s'\n", blueprint.properties[i].propertyName.AsString().AsCharPtr(), blueprint.name.Value());
				failed = true;
			}
		}

		if (!failed)
		{
			// Create the category table. This is just to that we always have access to the category directly
			CategoryId cid = EntityManager::Instance()->CreateCategory(info);
			Category const& cat = EntityManager::Instance()->GetCategory(cid);

			// Create the blueprint's template table
			MemDb::TableId tid = Game::GetWorldDatabase()->CreateTable(info);

			blueprint.categoryHash = cat.hash;
			blueprint.categoryId = cid;

			blueprint.tableId = tid;
			this->blueprintMap.Add(blueprint.name, idxBluePrint);
		}
	}

	if (failed)
	{
		n_error("Aborting due to unrecoverable error(s)!\n");
	}
}


} // namespace Game


