//------------------------------------------------------------------------------
//  @file propertyrecorder.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/extensions/propertyrecorder.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/gizmo.h"
#include "neweditor/commands/propertycommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/model/scene.h"
#include "game/levelextension.h"
#include "game/world.h"
#include "imgui.h"
#include "util/delegate.h"

namespace Editor
{

Util::Array<PendingPropertyChange> PropertyRecorder::pending;

//------------------------------------------------------------------------------
/**
    Whether the chain has the node the path names. A removal is only something the instance has to say
    when the node came from the chain: a node the instance authored on its own is gone with the keys
    that named it, and there is nothing left to mark.
*/
static bool
ChainHasNode(Scene* scene, Util::StringAtom extension, Game::Entity entity, const Util::String& path)
{
    Util::String chainText;
    Util::Array<Util::String> chainKeys;
    scene->GetChainExtension(entity, extension, chainText, chainKeys);

    const Util::String prefix = path + ".";
    for (IndexT i = 0; i < chainKeys.Size(); i++)
    {
        if (chainKeys[i] == path || chainKeys[i].BeginsWithString(prefix))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The keys the entity has authored after the change. A path with no field is one node: either
    everything the new text carries for it, which is an add, or nothing, which is a removal.

    Only the extension knows which keys a text carries, so this is where the editor learns what a
    key means.

    Answers true when the node was there before the change and is gone after it. That is the case
    where the instance took a node off its chain, and the caller is the one that can tell whether
    the node was the instance's own or the chain's.
*/
static bool
ApplyChangeToKeys(Util::Array<Util::String>& fields, const Util::String& path,
    const Util::String& oldText, const Util::String& newText, const Game::LevelExtension* extension,
    bool chainHasNode)
{
    const IndexT dot = path.FindCharIndex('.');

    if (dot == InvalidIndex)
    {
        const Util::String prefix = path + ".";

        bool wasThere = false;
        if (extension != nullptr && extension->keys != nullptr)
        {
            Util::Array<Util::String> before;
            extension->keys(oldText.AsCharPtr(), before);
            for (IndexT i = 0; i < before.Size(); i++)
            {
                if (before[i].BeginsWithString(prefix))
                {
                    wasThere = true;
                    break;
                }
            }
        }

        if (wasThere)
        {
            // The node is gone, and the keys that named it go with it
            for (IndexT i = (IndexT)fields.Size() - 1; i >= 0; i--)
            {
                if (fields[i].BeginsWithString(prefix))
                    fields.EraseIndex(i);
            }

            // A node the chain has is still the chain's, so the instance has to keep saying it is
            // absent. The key that says that is the path with no field
            if (chainHasNode)
            {
                bool already = false;
                for (IndexT f = 0; f < fields.Size(); f++)
                {
                    if (fields[f] == path)
                    {
                        already = true;
                        break;
                    }
                }
                if (!already)
                    fields.Append(path);
            }

            return true;
        }

        // An add. Every key the new text carries is the entity's own
        if (extension != nullptr && extension->keys != nullptr)
        {
            Util::Array<Util::String> keys;
            extension->keys(newText.AsCharPtr(), keys);
            for (IndexT i = 0; i < keys.Size(); i++)
            {
                if (!keys[i].BeginsWithString(prefix))
                    continue;

                bool already = false;
                for (IndexT f = 0; f < fields.Size(); f++)
                {
                    if (fields[f] == keys[i])
                    {
                        already = true;
                        break;
                    }
                }
                if (!already)
                    fields.Append(keys[i]);
            }
        }
        return false;
    }

    bool already = false;
    for (IndexT f = 0; f < fields.Size(); f++)
    {
        if (fields[f] == path)
        {
            already = true;
            break;
        }
    }
    if (!already)
        fields.Append(path);

    return false;
}

//------------------------------------------------------------------------------
/**
    A change a feature reported. The feature worked out the text before and after but wrote nothing,
    so the command is built here.

    The entity has to belong to the scene being edited, so a change on a scene behind a tab lands on
    that scene's entity rather than on the active one.

    This runs during the draw, so the change is queued.
*/
void
PropertyRecorder::RecordChange(Util::StringAtom extension, Game::Entity entity, const Util::String& path,
    const Util::String& oldText, const Util::String& newText)
{
    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr)
        return;

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return;

    if (entity.world != scene->GetAuthoringWorld()->GetWorldId())
        return;

    const Game::LevelExtension* ext = Game::LevelExtensionRegistry::GetExtension(extension);

    // Only a linked entity has a mark. An entity that is its own authored everything, so there is
    // nothing to
    // say about which part it wrote, and the command writes an empty mark for it
    const bool linked = scene->IsPrefabInstanceEntity(entity);

    // A path with no field is a whole node, which is the only thing an instance can take off its
    // chain. Whether the node came from the chain is what tells a removal from an instance's own
    // property going away, so the chain is only asked about for that path
    const bool chainHasNode = linked && path.FindCharIndex('.') == InvalidIndex &&
        ChainHasNode(scene, extension, entity, path);

    // One entry per entity, because a drag reports on every frame. Overwriting the entry is what
    // makes a drag
    // one command, and the before text stays the one from the first frame
    bool found = false;
    for (IndexT i = 0; i < PropertyRecorder::pending.Size(); i++)
    {
        PendingPropertyChange& existing = PropertyRecorder::pending[i];
        if (existing.entity == entity && existing.extension == extension)
        {
            existing.path = path;
            existing.newText = newText;
            if (linked)
                ApplyChangeToKeys(existing.newFields, path, existing.oldText, newText, ext, chainHasNode);
            found = true;
            break;
        }
    }

    if (!found)
    {
        PendingPropertyChange change;
        change.entity = entity;
        change.extension = extension;
        change.path = path;
        change.oldText = oldText;
        change.newText = newText;

        if (linked)
        {
            // The command writes the mark whole, so the keys the entity had before are carried
            // over. Starting
            // from empty would lose the ones authored earlier, and those keys would start following
            // the prefab
            change.oldFields = scene->FindExtensionOverride(entity, extension).fields;
            change.newFields = change.oldFields;
            ApplyChangeToKeys(change.newFields, path, oldText, newText, ext, chainHasNode);
        }

        PropertyRecorder::pending.Append(change);

        // Queued once per frame, because the later entries only change what this one already queued
        core->QueueAction(Util::Delegate<void()>::FromFunction<&PropertyRecorder::ApplyPending>());
    }
}

//------------------------------------------------------------------------------
/**
    The key a feature pressed its override mark on. A press does not say what it means, so the
    revert is composed here.

    The value the prefab chain gives for the key goes over what the entity has. A key the chain does
    not have goes back to the field's default, which only the extension knows, and that key stays
    marked, because its value is still the instance's own and the next save has to write it.

    When the whole extension is given back and the chain never had it, there is nothing to give it
    back to, which the command reads as the extension being gone.

    A key with no field names a whole node the instance took off its chain. Reverting that key gives
    the node back, and the key goes with it because the value is the chain's again.

    This runs during the draw, so the change is queued.
*/
void
PropertyRecorder::RequestRevert(Util::StringAtom extension, Game::Entity entity, const Util::String& path)
{
    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr)
        return;

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return;

    if (entity.world != scene->GetAuthoringWorld()->GetWorldId())
        return;

    // Only a linked entity has a mark, and a mark is the only thing a revert takes back
    if (!scene->IsPrefabInstanceEntity(entity))
        return;

    const Game::LevelExtension* ext = Game::LevelExtensionRegistry::GetExtension(extension);
    if (ext == nullptr || ext->keys == nullptr || ext->resolve == nullptr || ext->merge == nullptr)
        return;

    const PrefabOverride& state = scene->FindExtensionOverride(entity, extension);
    if (state.extension != extension)
        return;

    const Util::String single(path);
    const Util::String prefix = Util::String(path) + ".";
    const bool wholeProperty = single.FindCharIndex('.') == InvalidIndex;

    // The keys this extension's revert takes back
    Util::Array<Util::String> taken;
    for (IndexT i = 0; i < state.fields.Size(); i++)
    {
        if (wholeProperty)
        {
            // The key naming the node itself is one the revert takes back too, because that is the
            // mark saying the instance took the node off the chain
            if (state.fields[i] != single && !state.fields[i].BeginsWithString(prefix))
                continue;
        }
        else if (state.fields[i] != single)
            continue;

        taken.Append(state.fields[i]);
    }
    if (taken.IsEmpty())
        return;

    Util::String oldText;
    if (ext->capture != nullptr)
        ext->capture(entity, oldText);

    Util::Array<Util::String> allKeys;
    ext->keys(oldText.AsCharPtr(), allKeys);

    // The keys the entity keeps after the revert. It is every key its text carries and not only the
    // marked
    // ones, because an exact apply takes off whatever the composed text does not name
    Util::Array<Util::String> remaining;
    for (IndexT i = 0; i < allKeys.Size(); i++)
    {
        bool took = false;
        for (IndexT t = 0; t < taken.Size(); t++)
        {
            if (taken[t] == allKeys[i])
            {
                took = true;
                break;
            }
        }
        if (!took)
            remaining.Append(allKeys[i]);
    }

    Util::String chainText;
    Util::Array<Util::String> chainKeys;
    scene->GetChainExtension(entity, extension, chainText, chainKeys);

    // The keys the chain can answer for. The rest go back to the field's default, except a whole
    // extension the
    // chain never had, which has nothing to go back to. A key with no field names the whole node, and
    // the chain answers for it with the node it has
    Util::Array<Util::String> fromChain;
    for (IndexT i = 0; i < taken.Size(); i++)
    {
        const IndexT dot = taken[i].FindCharIndex('.');

        Util::String name;
        if (dot == InvalidIndex)
            name = taken[i];
        else
            name.Set(taken[i].AsCharPtr(), (SizeT)dot);

        bool chainHas = false;
        for (IndexT k = 0; k < chainKeys.Size(); k++)
        {
            const IndexT kdot = chainKeys[k].FindCharIndex('.');
            if (kdot == InvalidIndex)
            {
                if (chainKeys[k] == name)
                {
                    chainHas = true;
                    break;
                }
                continue;
            }

            Util::String kname;
            kname.Set(chainKeys[k].AsCharPtr(), (SizeT)kdot);
            if (kname == name)
            {
                chainHas = true;
                break;
            }
        }

        if (!chainHas && wholeProperty)
            continue;

        fromChain.Append(taken[i]);
    }

    // The mark after the revert. A key the chain answered for is no longer the instance's own. A
    // key it could
    // not answer for still is, because its value is the field's default, so the next save writes
    // it. A whole
    // extension the chain never had is dropped with the keys that named it, except the key naming the
    // node itself, which is the instance saying the node is absent and stays until the chain answers
    Util::Array<Util::String> newFields;
    for (IndexT i = 0; i < state.fields.Size(); i++)
    {
        bool took = false;
        for (IndexT t = 0; t < taken.Size(); t++)
        {
            if (taken[t] == state.fields[i])
            {
                took = true;
                break;
            }
        }
        if (!took)
        {
            newFields.Append(state.fields[i]);
            continue;
        }

        bool answered = false;
        for (IndexT f = 0; f < fromChain.Size(); f++)
        {
            if (fromChain[f] == state.fields[i])
            {
                answered = true;
                break;
            }
        }
        if (answered)
            continue;

        if (wholeProperty && state.fields[i].FindCharIndex('.') != InvalidIndex)
            continue;

        newFields.Append(state.fields[i]);
    }

    Util::String resolved;
    ext->resolve(chainText.AsCharPtr(), fromChain, resolved);

    Util::String capture;
    scene->CaptureExtensionKeys(entity, extension, remaining, capture);

    Util::String newText;
    ext->merge(resolved.AsCharPtr(), capture.AsCharPtr(), newText);

    PendingPropertyChange change;
    change.entity = entity;
    change.extension = extension;
    change.path = single;
    change.oldText = oldText;
    change.newText = newText;
    change.oldFields = state.fields;
    change.newFields = newFields;
    change.isRevert = true;

    PropertyRecorder::pending.Append(change);

    core->QueueAction(Util::Delegate<void()>::FromFunction<&PropertyRecorder::ApplyPending>());
}

//------------------------------------------------------------------------------
/**
    Whether the mark says this entity wrote the key. The mark is the scene's, so this is how a row
    asks.
*/
bool
PropertyRecorder::IsAuthored(Util::StringAtom extension, Game::Entity entity, const Util::String& path)
{
    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr)
        return false;

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return false;

    return scene->IsKeyAuthored(entity, extension, path);
}

//------------------------------------------------------------------------------
/**
    The queued changes are run as commands, which is what makes them undoable. Running them here
    instead of in the report keeps the draw from changing the arrays it is walking.
*/
void
PropertyRecorder::ApplyPending()
{
    Shell* core = EditorFeatureRegistry::GetShell();
    if (core == nullptr)
        return;

    if (PropertyRecorder::pending.IsEmpty())
        return;

    // A change is not run while a widget is still working on its value, which is until the button
    // is released.
    // The report only comes on the frame the widget changed, so the action has to go back on the
    // queue for the
    // next frame. Without that a deferred change is never run, and the widget shows a value for one
    // frame and
    // then reads it back from an entity that never got it
    //
    // A gizmo handle is not an ImGui item, so IsAnyItemActive() cannot see it
    if (ImGui::IsAnyItemActive() || Gizmo::IsDragging())
    {
        core->QueueAction(Util::Delegate<void()>::FromFunction<&PropertyRecorder::ApplyPending>());
        return;
    }

    Ptr<Scene> scene = core->GetScene();
    if (scene == nullptr)
        return;

    Ptr<EditCommandManager> manager = core->GetCommandManager();

    for (IndexT i = 0; i < PropertyRecorder::pending.Size(); i++)
    {
        const PendingPropertyChange& change = PropertyRecorder::pending[i];

        Ptr<PropertyCommand> command = PropertyCommand::Create();
        command->scene = scene;
        command->entity = change.entity;
        command->extension = change.extension;
        command->path = change.path;
        command->oldText = change.oldText;
        command->newText = change.newText;
        command->oldFields = change.oldFields;
        command->newFields = change.newFields;
        command->isRevert = change.isRevert;

        manager->Execute(command);
    }

    PropertyRecorder::pending.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
PropertyRecorder::Discard()
{
    PropertyRecorder::pending.Clear();
}

} // namespace Editor
//------------------------------------------------------------------------------
