#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::Clipboard

    The editor's copy and paste.

    The clipboard contains methods for storing the scene file format as a json string, which makes
    it readable by anything: the editor can read it back through the same parser it reads files
    with, a file can be pasted into the editor, and what the editor copies can be pasted into a file
    or into another application.

    @see
    Editor::SceneDocument

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "core/refcounted.h"
#include "neweditor/model/scenedocument.h"
#include "game/entity.h"
#include "util/array.h"
#include "util/guid.h"
#include "util/string.h"

namespace Editor
{

class Shell;
class Scene;

class Clipboard : public Core::RefCounted
{
    __DeclareClass(Clipboard)
public:
    Clipboard();
    virtual ~Clipboard();

    /// the core the clipboard reports to
    void Setup(Shell* core);
    /// clear the clipboard
    void Discard();

    /// copy the entities to the system clipboard. False when there is nothing to copy
    bool Copy(const Util::Array<Game::Entity>& entities);

    /// read the system clipboard and try to parse it as a scene file. Null when there is nothing to
    /// parse, which is what a paste from something that is not a scene file answers with
    Ptr<SceneDocument> Read() const;

    /// paste a document into the scene. The collection is the active one when it is invalid, and
    /// parent is what the paste's roots go under when it is given.
    bool Paste(const Ptr<SceneDocument>& doc, const Util::Guid& collection, Game::Entity parent,
        const char* action);

    /// duplicate the entities. This is a paste of what the entities make as a document, run against
    /// the document rather than against the clipboard
    bool Duplicate(const Util::Array<Game::Entity>& entities, const Util::Guid& collection,
        Game::Entity parent);

    /// the entities as a document: the entities and the subtree under each of them.
    /// guidByEntityIndex
    /// is what the writer resolves the entity references in the component values against, and only
    /// the
    /// entities in the copy are in it, which makes a reference to something outside the copy not
    /// survive the copy
    static Ptr<SceneDocument> BuildDocument(Scene* scene, const Util::Array<Game::Entity>& entities,
        Util::Array<Util::Guid>& guidByEntityIndex);

private:
    Shell* core;
};

} // namespace Editor
