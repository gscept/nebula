//------------------------------------------------------------------------------
//  blueprintmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "blueprintmanager.h"
#include "entitymanager.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "game/propertyserialization.h"
#include "util/arraystack.h"

namespace Game
{

__ImplementSingleton(BlueprintManager)

Util::String BlueprintManager::blueprintFolder("data:tables/");
Util::String BlueprintManager::templatesFolder("data:tables/templates");

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
	Util::Array<Util::String> files = IO::IoServer::Instance()->ListFiles(this->blueprintFolder, "*.json", true);
	for (int i = 0; i < files.Size(); i++)
	{
		if (!this->ParseBlueprint(files[i]))
		{
			n_warning("Warning: Managers::BlueprintManager: Error parsing %s!\n", files[i].AsCharPtr());
		}
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
	
	// parse all templates.
	if (IO::IoServer::Instance()->DirectoryExists(Singleton->templatesFolder))
	{
		Singleton->LoadTemplateFolder(Singleton->templatesFolder);
	}
}

//------------------------------------------------------------------------------
/**
	This method parses the file data:tables/blueprints.json into
	the blueprints array.
*/
bool
BlueprintManager::ParseBlueprint(Util::String const& blueprintsPath)
{
	if (IO::IoServer::Instance()->FileExists(blueprintsPath))
	{
		Ptr<IO::JsonReader> jsonReader = IO::JsonReader::Create();
		jsonReader->SetStream(IO::IoServer::Instance()->CreateStream(blueprintsPath));
		if (jsonReader->Open())
		{
			// make sure it's a BluePrints file
			if (!jsonReader->SetToNode("blueprints"))
			{
				n_warning("Warning: BlueprintManager::ParseBlueprints(): not a valid blueprints file!\n");
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

			} while (jsonReader->SetToNextChild());

			jsonReader->Close();
			return true;
		}
		else
		{
			n_error("Managers::BlueprintManager::ParseBlueprint(): could not open '%s'!", blueprintsPath.AsCharPtr());
			return false;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
BlueprintManager::LoadTemplateFolder(Util::String const& path)
{
	Util::Array<Util::String> files = IO::IoServer::Instance()->ListFiles(path, "*.json", true);

	// Parse files
	for (int i = 0; i < files.Size(); i++)
	{
		if (!this->ParseTemplate(files[i]))
		{
			n_warning("Managers::BlueprintManager: Error parsing %s!\n", files[i].AsCharPtr());
		}
	}

	// Recurse all folders
	Util::Array<Util::String> dirs = IO::IoServer::Instance()->ListDirectories(path, "*", true);
	for (auto const& dir : dirs)
		this->LoadTemplateFolder(dir);

	return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
BlueprintManager::ParseTemplate(Util::String const& templatePath)
{
	if (IO::IoServer::Instance()->FileExists(templatePath))
	{
		Ptr<IO::JsonReader> jsonReader = IO::JsonReader::Create();
		jsonReader->SetStream(IO::IoServer::Instance()->CreateStream(templatePath));
		if (jsonReader->Open())
		{
			jsonReader->SetToRoot();
			if (!jsonReader->HasNode("blueprint"))
			{
				jsonReader->Close();
				return false;
			}
			Util::StringAtom blueprintName = jsonReader->GetString("blueprint");
			if (this->blueprintMap.Contains(blueprintName))
			{
				// Instantiate template
				BlueprintId blueprint = this->blueprintMap[blueprintName];
				MemDb::TableId templateTid = this->blueprints[blueprint.id].tableId;
				IndexT instance = GetWorldDatabase()->AllocateRow(templateTid);

				n_assert2(instance < 0xFFFF, "Maximum number of templates per blueprint reached! You win!");

				// Create template name
				Util::String fileName = templatePath.ExtractFileName();
				fileName.StripFileExtension();
				Util::String templateName;
				templateName.Append(blueprintName.Value());
				templateName += "/" + fileName;

				TemplateId templateId;
				templateId.blueprintId = blueprint.id;
				templateId.templateId = instance;

				// Add to map
				this->templateMap.Add(Util::StringAtom(templateName), templateId);

				// Override properties if necessary
				if (jsonReader->SetToFirstChild("properties"))
				{
					jsonReader->SetToFirstChild();
					do
					{
						Util::StringAtom propertyName = jsonReader->GetCurrentNodeName();
						MemDb::ColumnDescriptor descriptor = MemDb::TypeRegistry::GetDescriptor(propertyName);
						if (descriptor == MemDb::ColumnDescriptor::Invalid())
						{
							n_warning("Warning: Template contains invalid property named '%s'. (%s)\n", propertyName.Value(), templatePath.AsCharPtr());
							continue;
						}

						MemDb::ColumnId column = GetWorldDatabase()->GetColumnId(templateTid, descriptor);
						if (column == MemDb::ColumnId::Invalid())
						{
							n_warning("Warning: Template contains property named '%s' that does not exist in blueprint. (%s)\n", propertyName.Value(), templatePath.AsCharPtr());
							continue;
						}

						void* propertyValue = GetWorldDatabase()->GetValuePointer(templateTid, column, instance);
						PropertySerialization::Deserialize(jsonReader, propertyName, propertyValue);
					} while (jsonReader->SetToNextChild());

					jsonReader->SetToParent();
				}

				// TODO: Create template in blueprint table

				if (jsonReader->SetToFirstChild("variations"))
				{
					jsonReader->SetToFirstChild();
					do
					{
						Util::StringAtom propertyName = jsonReader->GetCurrentNodeName();
						// TODO: GetValue!
					} while (jsonReader->SetToNextChild());

					// TODO: Create template variations in blueprint table

					jsonReader->SetToParent();
				}
			}
			jsonReader->Close();
			return true;
		}
		else
		{
			n_error("Managers::BlueprintManager::ParseTemplate(): could not open '%s'!", templatePath.AsCharPtr());
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
	blueprintFolder = folder;
}

//------------------------------------------------------------------------------
/**
*/
BlueprintId const
BlueprintManager::GetBlueprintId(Util::StringAtom name)
{
	n_assert(Singleton != nullptr);
	return Singleton->blueprintMap[name];
}

//------------------------------------------------------------------------------
/**
*/
TemplateId const
BlueprintManager::GetTemplateId(Util::StringAtom name)
{
	IndexT index = Singleton->templateMap.FindIndex(name);
	if (index != InvalidIndex)
	{
		TemplateId tid = Singleton->templateMap.ValueAtIndex(name, index);
		n_assert(Singleton->blueprints.Size() > tid.blueprintId);
		return tid;
	}
	else
	{
		return TemplateId::Invalid();
	}
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
BlueprintManager::Instantiate(TemplateId templateId)
{
	EntityManager::State const& emState = EntityManager::Instance()->state;
	Ptr<MemDb::Database> const& db = emState.worldDatabase;
	CategoryId cid = Singleton->blueprints[templateId.blueprintId].categoryId;
	Category& cat = emState.categoryArray[cid.id];
	InstanceId instance = db->DuplicateInstance(Singleton->blueprints[templateId.blueprintId].tableId, templateId.templateId, cat.instanceTable);
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

		const SizeT numBlueprintProperties = blueprint.properties.Size();
		Util::ArrayStack<PropertyId, 32> columns;

		for (int i = 0; i < numBlueprintProperties; i++)
		{
			auto descriptor = MemDb::TypeRegistry::GetDescriptor(blueprint.properties[i].propertyName);
			if (descriptor != PropertyId::Invalid())
			{
				// append to dynamically resizable array
				columns.Append(descriptor);
			}
			else
			{
				n_warning("Warning: Unrecognized property '%s' in blueprint '%s'\n", blueprint.properties[i].propertyName.AsString().AsCharPtr(), blueprint.name.Value());
			}
		}

		// move properties from dynamically sized array to fixed array.
		// this is kinda wonky, but then we don't need to do anything special with invalid properties...
		info.columns.SetSize(columns.Size());
		for (int i = 0; i < columns.Size(); i++)
		{
			info.columns[i] = columns[i];
		}

		if (!failed)
		{
			// Create the category table. This is just to that we always have access to the category directly
			CategoryId cid = EntityManager::Instance()->CreateCategory(info);
			Category const& cat = EntityManager::Instance()->GetCategory(cid);

			// Create the blueprint's template table
			info.name = "blueprint:" + info.name;
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


