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
    if (!this->ParseBluePrints())
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
	the bluePrints array.
*/
bool
BlueprintManager::ParseBluePrints()
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
				n_error("BlueprintManager::ParseBluePrints(): not a valid blueprints file!");
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
			n_error("Managers::BlueprintManager::ParseBluePrints(): could not open '%s'!", blueprintsPath.AsCharPtr());
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
void
BlueprintManager::SetupCategories()
{

	// create a instance of every property and call SetupDefaultAttributes()
	IndexT idxBluePrint;
	bool failed = false;
	for (idxBluePrint = 0; idxBluePrint < this->bluePrints.Size(); idxBluePrint++)
	{
		const BluePrint& bluePrint = this->bluePrints[idxBluePrint];

		if (Game::CategoryExists(bluePrint.name))
		{
			n_warning("Duplicate blueprint named '%s' found in blueprints.json\n", bluePrint.name.AsCharPtr());
			continue;
		}
		CategoryCreateInfo info;
		info.name = bluePrint.name;

		const SizeT numCols = bluePrint.properties.Size();
		info.columns.SetSize(numCols);

		for (int i = 0; i < numCols; i++)
		{
			auto descriptor = MemDb::TypeRegistry::GetDescriptor(bluePrint.properties[i].propertyName);
			if (descriptor != PropertyId::Invalid())
			{
				info.columns[i] = descriptor;
			}
			else
			{
				n_printf("Error: Unrecognized property '%s' in blueprint '%s'\n", bluePrint.properties[i].propertyName.AsString().AsCharPtr(), bluePrint.name.AsCharPtr());
				failed = true;
			}
		}

		if (!failed)
			EntityManager::Instance()->AddCategory(info);
	}

	if (failed)
	{
		n_error("Aborting due to unrecoverable error(s)!\n");
	}
}


} // namespace Game


