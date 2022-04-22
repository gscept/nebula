//------------------------------------------------------------------------------
//  entityloader.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "entityloader.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "io/ioserver.h"
#include "editor.h"
#include "cmds.h"
#include "commandmanager.h"
#include "game/componentserialization.h"
#include "basegamefeature/managers/blueprintmanager.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
    Entity format is defined as:
    \code{.json}
    {
        "entities": {
            "[GUID]": {
                "name": String,
                "template": "TemplateName", // this should be changed to a GUID probably
                "components": { // only lists the overridden and additional components
                    "ComponentName1": VALUE,
                    "ComponentName2": {
                        "FieldName1": VALUE,
                        "FieldName2": VALUE
                    }
                },
                "inactive_properties": {
                    ...
                },
                "removed_components": { // lists components that have been removed from the entity, that are part of the blueprint.
                    "ComponentName3",
                    "ComponentName4"
                }
            },
            "[GUID]": {
                ...
            }
        }
    }
    \endcode
*/
bool
LoadEntities(const char* filePath)
{
    IO::URI const file = filePath;
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(IO::IoServer::Instance()->CreateStream(file));
    if (reader->Open())
    {
        reader->SetToFirstChild("entities");
        Util::Blob scratchBuffer = Util::Blob(128);
        Edit::CommandManager::BeginMacro("Load entities", false);
        reader->SetToFirstChild();
        do
        {
            Editor::Entity editorEntity;
            Util::String entityName = reader->GetOptString("name", "unnamed_entity");
            bool const fromTemplate = reader->HasNode("template");
            if (fromTemplate)
            {
                Util::String templateName = reader->GetString("template");
                editorEntity = Edit::CreateEntity(templateName);
            }
            else
            {
                editorEntity = Edit::CreateEntity("Empty/empty");
            }

            Edit::SetEntityName(editorEntity, entityName);

            Util::String const guid = reader->GetCurrentNodeName();
            Editor::state.editables[editorEntity.index].guid = Util::Guid::FromString(guid);

            if (reader->SetToFirstChild("components"))
            {
                if (reader->HasChildren())
                {
                    reader->SetToFirstChild();
                    do
                    {
                        Util::StringAtom const componentName = reader->GetCurrentNodeName();
                        MemDb::ComponentId descriptor = MemDb::TypeRegistry::GetComponentId(componentName);
                        if (descriptor == MemDb::ComponentId::Invalid())
                        {
                            n_warning("Warning: Entity '%s' contains invalid component named '%s'.\n", entityName.AsCharPtr(), componentName.Value());
                            continue;
                        }


                        if (!Game::HasComponent(Editor::state.editorWorld, editorEntity, descriptor))
                        {
                            Edit::AddComponent(editorEntity, descriptor);
                        }

                        if (MemDb::TypeRegistry::TypeSize(descriptor) > 0)
                        {
                            if (scratchBuffer.Size() < MemDb::TypeRegistry::TypeSize(descriptor))
                                scratchBuffer.Reserve(MemDb::TypeRegistry::TypeSize(descriptor));
                            Game::ComponentSerialization::Deserialize(reader, descriptor, scratchBuffer.GetPtr());
                            Edit::SetComponent(editorEntity, descriptor, scratchBuffer.GetPtr());
                        }
                    } while (reader->SetToNextChild());
                }
                reader->SetToParent();
            }
            if (reader->SetToFirstChild("removed_components"))
            {
                n_assert(reader->IsArray());
                
                Util::Array<Util::String> values;
                reader->Get<Util::Array<Util::String>>(values);
                for (auto s : values)
                {
                    Game::ComponentId const component = MemDb::TypeRegistry::GetComponentId(s);
                    if (component != Game::ComponentId::Invalid())
                        Edit::RemoveComponent(editorEntity, component);
                }
                
                reader->SetToParent();
            }
        } while (reader->SetToNextChild());
        Edit::CommandManager::EndMacro();
        reader->Close();
        return true;
    }
    return false;
} 

//------------------------------------------------------------------------------
/**
*/
bool
SaveEntities(const char* filePath)
{
    IO::URI const file = filePath;
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(IO::IoServer::Instance()->CreateStream(file));
    if (writer->Open())
    {
        writer->BeginObject("entities");

        Game::FilterBuilder::FilterCreateInfo filterInfo;
        filterInfo.inclusive[0] = Game::GetComponentId("Owner"_atm);
        filterInfo.access[0] = Game::AccessMode::READ;
        filterInfo.numInclusive = 1;

        Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
        Game::Dataset data = Game::Query(state.editorWorld, filter);
        Game::ComponentId ownerPid = Game::GetComponentId("Owner"_atm);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];
            Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Editor::Entity const& editorEntity = entities[i];
                Editable& edit = state.editables[editorEntity.index];
                Game::EntityMapping const mapping = Game::GetEntityMapping(Editor::state.editorWorld, editorEntity);
                writer->BeginObject(edit.guid.AsString().AsCharPtr());
                writer->Add(edit.name, "name");
                if (edit.templateId != Game::TemplateId::Invalid())
                {
                    writer->Add(Game::BlueprintManager::GetTemplateName(edit.templateId).Value(), "template");
                }
                MemDb::Table const& table = Game::GetWorldDatabase(Editor::state.editorWorld)->GetTable(view.tableId);
                IndexT col = 0;
                if (table.components.Size() > 1)
                {
                    writer->BeginObject("components");
                    for (auto component : table.components)
                    {
                        uint32_t const flags = MemDb::TypeRegistry::Flags(component);
                        if (component != ownerPid && (flags & Game::ComponentFlags::COMPONENTFLAG_MANAGED) == 0)
                        {
                            SizeT const typeSize = MemDb::TypeRegistry::TypeSize(component);
                            if (typeSize > 0)
                            {
                                void* buffer = Game::GetInstanceBuffer(Editor::state.editorWorld, table.tid, component);
                                n_assert(buffer != nullptr);
                                Game::ComponentSerialization::Serialize(writer, component, ((byte*)buffer) + MemDb::TypeRegistry::TypeSize(component) * mapping.instance);
                            }
                            else
                            {
                                writer->Add("null", MemDb::TypeRegistry::GetDescription(component)->name.Value());
                            }
                        }

                        col++;
                    }
                    writer->End();
                }
                writer->End();
            }
        }

        Game::DestroyFilter(filter);

        writer->End();
        return true;
    }

    return false;
}
} // namespace Editor
