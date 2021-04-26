//------------------------------------------------------------------------------
//  blueprintmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "blueprintmanager.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "game/propertyserialization.h"
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

                // Override properties if necessary
                if (jsonReader->SetToFirstChild("properties"))
                {
                    jsonReader->SetToFirstChild();
                    do
                    {
                        Util::StringAtom propertyName = jsonReader->GetCurrentNodeName();
                        MemDb::PropertyId descriptor = MemDb::TypeRegistry::GetPropertyId(propertyName);
                        if (descriptor == MemDb::PropertyId::Invalid())
                        {
                            n_warning("Warning: Template contains invalid property named '%s'. (%s)\n", propertyName.Value(), templatePath.AsCharPtr());
                            continue;
                        }

                        MemDb::ColumnIndex column = templateDatabase->GetColumnId(templateTid, descriptor);
                        if (column == MemDb::ColumnIndex::Invalid())
                        {
                            n_warning("Warning: Template contains property named '%s' that does not exist in blueprint. (%s)\n", propertyName.Value(), templatePath.AsCharPtr());
                            continue;
                        }

                        void* propertyValue = templateDatabase->GetValuePointer(templateTid, column, instance);
                        PropertySerialization::Deserialize(jsonReader, descriptor, propertyValue);
                    } while (jsonReader->SetToNextChild());

                    jsonReader->SetToParent();
                }

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
BlueprintManager::Instantiate(World* const world, BlueprintId blueprint)
{
    GameServer::State& gsState = GameServer::Instance()->state;
    Ptr<MemDb::Database> const& tdb = gsState.templateDatabase;
    IndexT const categoryIndex = world->blueprintCatMap.FindIndex(blueprint);

    if (categoryIndex != InvalidIndex)
    {
        MemDb::TableId const cid = world->blueprintCatMap.ValueAtIndex(blueprint, categoryIndex);
        MemDb::Row const instance = world->db->AllocateRow(cid);
        return { cid, instance };
    }
    else
    {
        // Create the category, and then create the instance
        MemDb::TableId const cid = this->CreateCategory(world, blueprint);
        MemDb::Row const instance = world->db->AllocateRow(cid);
        return { cid, instance };
    }
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
BlueprintManager::Instantiate(World* const world, TemplateId templateId)
{
    n_assert(Singleton->templateIdPool.IsValid(templateId.id));
    GameServer::State& gsState = GameServer::Instance()->state;
    Ptr<MemDb::Database> const& tdb = gsState.templateDatabase;
    Template& tmpl = Singleton->templates[Ids::Index(templateId.id)];
    IndexT const categoryIndex = world->blueprintCatMap.FindIndex(tmpl.bid);
    
    if (categoryIndex != InvalidIndex)
    {
        MemDb::TableId const cid = world->blueprintCatMap.ValueAtIndex(tmpl.bid, categoryIndex);
        MemDb::Row const instance = tdb->DuplicateInstance(Singleton->blueprints[tmpl.bid.id].tableId, tmpl.row, world->db, cid);
        return { cid, instance };
    }
    else
    {
        // Create the category, and then create the instance
        MemDb::TableId const cid = this->CreateCategory(world, tmpl.bid);
        MemDb::Row const instance = tdb->DuplicateInstance(Singleton->blueprints[tmpl.bid.id].tableId, tmpl.row, world->db, cid);
        return { cid, instance };
    }
}

//------------------------------------------------------------------------------
/**
*/
void
BlueprintManager::SetupBlueprints()
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
        CategoryCreateInfo info;
        info.name = blueprint.name.AsString();

        const SizeT numBlueprintProperties = blueprint.properties.Size();
        Util::ArrayStack<PropertyId, 32> columns;

        // append owner, makes it a bit faster than letting entitymanager sort it out...
        columns.Append(GameServer::Instance()->state.ownerId);

        // filter out invalid properties
        for (int i = 0; i < numBlueprintProperties; i++)
        {
            auto descriptor = MemDb::TypeRegistry::GetPropertyId(blueprint.properties[i].propertyName);
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
        info.properties.SetSize(columns.Size());
        for (int i = 0; i < columns.Size(); i++)
        {
            info.properties[i] = columns[i];
        }

        if (!failed)
        {
            // Create the blueprint's template table
            MemDb::TableCreateInfo tableInfo;
            tableInfo.name = "blueprint:" + info.name;
            tableInfo.numProperties = info.properties.Size();
            tableInfo.properties = info.properties.Begin();
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
    @todo   this can be optimized
*/
MemDb::TableId
BlueprintManager::CreateCategory(World* const world, BlueprintId bid)
{
    CategoryCreateInfo info;
    info.name = blueprints[bid.id].name.Value();
    
    auto const& properties = GameServer::Singleton->state.templateDatabase->GetTable(blueprints[bid.id].tableId).properties;
    info.properties.Resize(properties.Size());
    for (int i = 0; i < properties.Size(); i++)
    {
        PropertyId p = properties[i];
        info.properties[i] = p;
    }
     
    return CreateEntityTable(world, info);
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


