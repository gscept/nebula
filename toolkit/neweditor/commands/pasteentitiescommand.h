#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::PasteEntitiesCommand

    Puts a document into a scene. This is what a paste and a duplicate run through

    @see
    Editor::Clipboard

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/scenedocument.h"
#include "neweditor/model/scene.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/blob.h"
#include "util/guid.h"
#include "util/string.h"

namespace Editor
{

class PasteEntitiesCommand : public EditCommand
{
    __DeclareClass(PasteEntitiesCommand)
public:
    virtual ~PasteEntitiesCommand();

    const char* Name() const override;
    bool Execute() override;
    bool Unexecute() override;

    Ptr<Scene> scene;
    /// the document being pasted
    Ptr<SceneDocument> document;
    /// the collection the entities go into. Invalid means the active one
    Util::Guid collection;
    /// the entity the paste's roots go under. Invalid means they keep no parent
    Game::Entity parent;
    /// what the undo stack and the log call the run, which is what lets a duplicate and a paste be
    /// told apart on the undo stack
    Util::String action;
    /// the entities the run made, in document order. The panels read it after a run to know what
    /// was
    /// made and which of them are the roots
    Util::Array<Game::Entity> created;

private:
    /// one entity of the paste, as it was while the paste is out
    struct EntitySnapshot
    {
        Editable record;
        Util::Array<Game::ComponentId> components;
        Util::Array<Util::Blob> values;
        /// the feature data the entity had, as the text the extensions produced for it
        Util::Array<DocumentExtension> extensions;
    };

    /// read the document and create. False when the document is empty or cannot be put into the
    /// scene
    bool PasteFromDocument();
    /// put the entities back from the snapshots, which is what a redo does
    void PasteFromSnapshots();

    Util::Array<EntitySnapshot> snapshots;
    bool initialized = false;
};

} // namespace Editor
