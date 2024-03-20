//------------------------------------------------------------------------------
//  @file level.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "level.h"
#include "io/jsonreader.h"
#include "io/jsonwriter.h"
#include "io/ioserver.h"
#include "editor/editor.h"

#include "basegamefeature/managers/blueprintmanager.h"
#include "flat/game/level.h"

namespace Editor
{
__ImplementClass(Editor::Level, 'ELVL', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
Level::Level()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Level::~Level()
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
Level::Clear()
{
}

//------------------------------------------------------------------------------
/**
*/
bool
Level::LoadLevel(const Util::String& name)
{
    /*
    IO::URI const file = name;
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
                uint numChildren = reader->CurrentSize();
                for (uint childIndex = 0; childIndex < numChildren; childIndex++)
                {
                    Util::String const componentName = reader->GetChildNodeName(childIndex);
                    MemDb::AttributeId attributeId = MemDb::AttributeRegistry::GetAttributeId(componentName);
                    if (attributeId == MemDb::AttributeId::Invalid())
                    {
                        n_warning(
                            "Warning: Entity '%s' contains invalid component named '%s'.\n",
                            entityName.AsCharPtr(),
                            componentName.AsCharPtr()
                        );
                        continue;
                    }

                    if (!Editor::state.editorWorld->HasComponent(editorEntity, attributeId))
                    {
                        Edit::AddComponent(editorEntity, attributeId);
                    }

                    SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(attributeId);
                    if (typeSize > 0)
                    {
                        if (scratchBuffer.Size() < typeSize)
                            scratchBuffer.Reserve(typeSize);

                        Game::ComponentSerialization::Deserialize(reader, attributeId, scratchBuffer.GetPtr());
                        Edit::SetComponent(editorEntity, attributeId, scratchBuffer.GetPtr());
                    }
                }

                reader->SetToParent();
            }
            this->entities.Append(editorEntity);
        } while (reader->SetToNextChild());
        Edit::CommandManager::EndMacro();
        reader->Close();
        return true;
    }
    */
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Level::SaveLevelAs(const Util::String& name)
{
    /*
    IO::URI const file = name;
    Ptr<IO::JsonWriter> writer = IO::JsonWriter::Create();
    writer->SetStream(IO::IoServer::Instance()->CreateStream(file));
    if (writer->Open())
    {
        writer->BeginObject("entities");

        Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
        Game::Dataset data = state.editorWorld->Query(filter);
        Game::ComponentId entityPID = Game::GetComponentId<Game::Entity>();

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Editor::Entity const* const entities = (Editor::Entity*)view.buffers[0];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Editor::Entity const& editorEntity = entities[i];
                Editable& edit = state.editables[editorEntity.index];
                Game::EntityMapping const mapping = Editor::state.editorWorld->GetEntityMapping(editorEntity);
                writer->BeginObject(edit.guid.AsString().AsCharPtr());
                writer->Add(edit.name, "name");
                if (edit.templateId != Game::TemplateId::Invalid())
                {
                    writer->Add(Game::BlueprintManager::GetTemplateName(edit.templateId).Value(), "template");
                }
                MemDb::Table const& table = Editor::state.editorWorld->GetDatabase()->GetTable(view.tableId);
                IndexT col = 0;
                if (table.GetAttributes().Size() > 1)
                {
                    writer->BeginObject("components");
                    for (auto component : table.GetAttributes())
                    {
                        uint32_t const flags = MemDb::AttributeRegistry::Flags(component);
                        if (component != entityPID && (flags & Game::ComponentFlags::COMPONENTFLAG_DECAY) == 0)
                        {
                            SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
                            if (typeSize > 0)
                            {
                                void* buffer = Editor::state.editorWorld->GetInstanceBuffer(
                                    view.tableId, mapping.instance.partition, component
                                );
                                n_assert(buffer != nullptr);
                                Game::ComponentSerialization::Serialize(
                                    writer,
                                    component,
                                    ((byte*)buffer) + MemDb::AttributeRegistry::TypeSize(component) * mapping.instance.index
                                );
                            }
                            else
                            {
                                writer->Add("null", MemDb::AttributeRegistry::GetAttribute(component)->name.Value());
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
    */
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Level::Export(const Util::String& name)
{
    Editor::state.editorWorld->ExportLevel(name);
    return true;
}

} // namespace Editor
