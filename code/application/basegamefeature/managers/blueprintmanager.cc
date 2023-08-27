//------------------------------------------------------------------------------
//  blueprintmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "blueprintmanager.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "game/componentserialization.h"
#include "util/arraystack.h"
#include "game/gameserver.h"

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
    Singleton = new BlueprintManager;

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
    delete Singleton;
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
    // first, setup an "empty" blueprint
    Blueprint bluePrint;
    bluePrint.name = "Empty";
    BlueprintId const emptyId = Singleton->blueprints.Size();
    Singleton->blueprints.Append(bluePrint);
    
    // Setup all blueprint tables
    Singleton->SetupBlueprints();
    
    // parse all templates from folders.
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

                if (jsonReader->SetToFirstChild("components"))
                {
                    if (jsonReader->IsArray())
                    {
                        Util::Array<Util::String> comps;
                        jsonReader->Get<>(comps);
                        for (Util::String const& name : comps)
                        {
                            bluePrint.components.Append({ name });
                        }
                    }
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
                Ptr<MemDb::Database> templateDatabase = GameServer::Instance()->state.templateDatabase;
                BlueprintId blueprint = this->blueprintMap[blueprintName];
                MemDb::TableId templateTid = this->blueprints[blueprint.id].tableId;
                IndexT instance = templateDatabase->AllocateRow(templateTid);
                n_assert2(instance < 0xFFFF, "Maximum number of templates per blueprint reached! You win!");

                // Create template name
                Util::String fileName = templatePath.ExtractFileName();
                fileName.StripFileExtension();
                Util::String templateName;
                templateName.Append(blueprintName.Value());
                templateName += "/" + fileName;

                Util::StringAtom const nameAtom = Util::StringAtom(templateName);

                TemplateId templateId;
                if (this->templateIdPool.Allocate(templateId.id))
                {
                    this->templates.Append({});
                }

                Template& tmpl = this->templates[Ids::Index(templateId.id)];
                tmpl = Template();
                tmpl.bid = blueprint;
                tmpl.name = nameAtom;
                tmpl.row = instance;
                
                // Add to map
                this->templateMap.Add(nameAtom, templateId);

                // Override components if necessary
                if (jsonReader->SetToFirstChild("components"))
                {
                    jsonReader->SetToFirstChild();
                    do
                    {
                        Util::StringAtom componentName = jsonReader->GetCurrentNodeName();
                        MemDb::ComponentId descriptor = MemDb::TypeRegistry::GetComponentId(componentName);
                        if (descriptor == MemDb::ComponentId::Invalid())
                        {
                            n_warning("Warning: Template contains invalid component named '%s'. (%s)\n", componentName.Value(), templatePath.AsCharPtr());
                            continue;
                        }

                        MemDb::ColumnIndex column = templateDatabase->GetColumnId(templateTid, descriptor);
                        if (column == MemDb::ColumnIndex::Invalid())
                        {
                            n_warning("Warning: Template contains component named '%s' that does not exist in blueprint. (%s)\n", componentName.Value(), templatePath.AsCharPtr());
                            continue;
                        }

                        void* componentValue = templateDatabase->GetValuePointer(templateTid, column, instance);
                        ComponentSerialization::Deserialize(jsonReader, descriptor, componentValue);
                    } while (jsonReader->SetToNextChild());

                    jsonReader->SetToParent();
                }

                if (jsonReader->SetToFirstChild("variations"))
                {
                    jsonReader->SetToFirstChild();
                    do
                    {
                        Util::StringAtom componentName = jsonReader->GetCurrentNodeName();
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
void
BlueprintManager::SetupBlueprints()
{
    // create a instance of every component and call SetupDefaultAttributes()
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
        CategoryCreateInfo info;
        info.name = blueprint.name.AsString();

        const SizeT numBlueprintComponents = blueprint.components.Size();
        Util::StackArray<ComponentId, 32> columns;

        // append owner, makes it a bit faster than letting entitymanager sort it out...
        columns.Append(GameServer::Instance()->state.ownerId);

        // filter out invalid components
        for (int i = 0; i < numBlueprintComponents; i++)
        {
            auto descriptor = MemDb::TypeRegistry::GetComponentId(blueprint.components[i].name);
            if (descriptor != ComponentId::Invalid())
            {
                // append to dynamically resizable array
                columns.Append(descriptor);
            }
            else
            {
                n_warning("Warning: Unrecognized component '%s' in blueprint '%s'\n", blueprint.components[i].name.AsString().AsCharPtr(), blueprint.name.Value());
            }
        }

        // move components from dynamically sized array to fixed array.
        // this is kinda wonky, but then we don't need to do anything special with invalid components...
        info.components.SetSize(columns.Size());
        for (int i = 0; i < columns.Size(); i++)
        {
            info.components[i] = columns[i];
        }

        if (!failed)
        {
            // Create the blueprint's template table
            MemDb::TableCreateInfo tableInfo;
            tableInfo.name = "blueprint:" + info.name;
            tableInfo.numComponents = info.components.Size();
            tableInfo.components = info.components.Begin();
            Ptr<MemDb::Database> templateDatabase = GameServer::Instance()->state.templateDatabase;
            MemDb::TableId tid = templateDatabase->CreateTable(tableInfo);

            blueprint.tableId = tid;
            this->blueprintMap.Add(blueprint.name, idxBluePrint);

            // Create a template for this blueprints default values
            IndexT instance = templateDatabase->AllocateRow(tid);
            TemplateId templateId;
            if (this->templateIdPool.Allocate(templateId.id))
                this->templates.Append({});
            Template& tmpl = this->templates[Ids::Index(templateId.id)];
            tmpl = Template();
            tmpl.bid = idxBluePrint;
            tmpl.name = blueprint.name;
            tmpl.row = instance;
            this->templateMap.Add(blueprint.name, templateId);
        }
    }

    if (failed)
    {
        n_error("Aborting due to unrecoverable error(s)!\n");
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::StringAtom const
BlueprintManager::GetTemplateName(TemplateId const templateId)
{
    n_assert(Singleton->templateIdPool.IsValid(templateId.id));
    return Singleton->templates[Ids::Index(templateId.id)].name;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<BlueprintManager::Template> const&
BlueprintManager::ListTemplates()
{
    return Singleton->templates;
}

} // namespace Game


