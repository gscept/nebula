#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::EditorFeatureRegistry

    A feature is a part of the game that the editor shows. The editor does not know what any feature
    is. A feature gives its parts to this registry when it starts, and the core uses them when it
    draws. A feature that gives nothing is never asked for, so a game is only the features it
    starts.

    The core asks for four things, because it draws them inside a window it owns: a section in the
    inspector, the entries of the add dropdown, the report that a component value changed, and the
    numbers on the status bar. A feature draws its own panel itself, once per frame. Its commands
    and keybinds go through their own registries, and its state between frames stays on it.

    The core answers its rules from data, not from a function. A feature gives the count rule as a
    set of components and a number, and the runtime rule as a list of component ids. The panel then
    answers both from the data, without knowing what a collider or an animation is.

    This registry is static. Every feature is added before the editor opens, so the core reads it at
    open, whatever order the features came in.

    @see
    Editor::Shell

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "game/componentid.h"
#include "game/entity.h"
#include "io/path.h"
#include "util/array.h"
#include "util/blob.h"
#include "util/delegate.h"
#include "util/string.h"
#include "util/stringatom.h"

namespace Editor
{

class EditorPanel;
class Scene;
class Shell;
struct ToolView;

/// The payload of an asset dragged in the editor, out of the asset browser or an asset field. It
/// carries the asset's URN as text, folder:file:type. The source and the targets live in different
/// modules, so the name is the editor's
static const char* const AssetDragPayload = "nebula.asset";

//------------------------------------------------------------------------------
/**
    One entry of the inspector's add dropdown. It is either a component, which the editor can read,
    or something else, which the editor cannot read.
*/
struct InspectorEntry
{
    Game::ComponentId component = Game::ComponentId::Invalid();
    /// the level extension the entry belongs to. Invalid for a component
    Util::StringAtom extension;
    /// the name the row shows
    Util::String name;
    /// the group the entry is drawn under. For a component this is its namespace, otherwise the
    /// name of the entry source
    Util::String group;
    /// true when the entry is a component
    bool isComponent = true;
    /// true when the entity already has the entry. The row is drawn disabled with a checkmark.
    bool present = false;
    /// true when the entity cannot be given the entry, for a reason other than already having it.
    /// The row is disabled as well, but without the checkmark.
    bool blocked = false;
    /// the text a new entry starts with. Only an entry the editor cannot read needs one, because
    /// the default for a component comes from reflection.
    Util::Delegate<void(const char* name, Util::String& out)> defaultText;
};

//------------------------------------------------------------------------------
/**
    A section the inspector draws for an entity. The core draws the entity's own rows first and then
    calls each section. The section draws its heading and its rows.
*/
struct EditorInspectorSection
{
    Util::Delegate<void(Game::Entity)> draw;
};

//------------------------------------------------------------------------------
/**
    An entry source: a function that returns the entries for one entity. The core cannot read what
    the function returns, so it calls it. The entries come back unfiltered, because only the panel
    does the text match.
*/
struct EditorInspectorAddEntrySource
{
    Util::Delegate<void(Game::Entity, Util::Array<InspectorEntry>&)> collect;
};

//------------------------------------------------------------------------------
/**
    How many of a set of components one entity may have. A feature states this as data, so the panel
    can answer the question itself.
*/
struct EditorAddComponentConstraint
{
    Util::Array<Game::ComponentId> components;
    IndexT maxPerEntity;
};

//------------------------------------------------------------------------------
/**
    The feature is told when the editor writes a component value the feature cannot read. The editor
    made the change, and a feature applies such a value only when someone tells it.
*/
struct EditorOnComponentChangeListener
{
    Util::Delegate<void(Scene*, Game::Entity, Game::ComponentId)> onChange;
};

//------------------------------------------------------------------------------
/**
    The numbers a feature adds to the status bar, after the frame time the core has on its own. The
    bar cannot read them: the draw calls come from the renderer, and the render resolution is the
    size of the target a feature draws into. The feature returns them as text, and the bar puts them
    on the same line as the message.
*/
struct EditorStatusNumbers
{
    Util::Delegate<void(Util::String& out)> append;
};

//------------------------------------------------------------------------------
/**
    A popup a feature draws itself, like a picker. It is drawn after the panels, with the core's own
    modals, so it ends up above the panel that opened it. The popup decides when it is open.
*/
struct EditorModal
{
    Util::Delegate<void()> draw;
};

//------------------------------------------------------------------------------
/**
    Work a feature holds that is not saved, like an asset open in its own editor. The exit question
    covers it along with the scenes: list names what has changes, and save writes them when the
    question is answered with Save. A save that fails returns false and the editor stays open.
*/
struct EditorUnsavedWork
{
    Util::Delegate<void(Util::Array<Util::String>& out)> list;
    Util::Delegate<bool()> save;
};

//------------------------------------------------------------------------------
/**
    What an asset dropped on the viewport is made into: one entity named after the asset, with the
    components each feature that takes the asset's type gives it. The viewport puts the entity where
    the drop lands, so a feature leaves the transform alone.
*/
struct EditorDroppedAsset
{
    /// the asset that was dropped, folder:file:type
    IO::Path asset;
    /// the components the entity is made with, and the value of each
    Util::Array<Game::ComponentId> components;
    Util::Array<Util::Blob> values;

    /// give the entity a component with a value. A component given twice keeps the last value
    void Add(Game::ComponentId component, const void* value, SizeT size);
};

//------------------------------------------------------------------------------
/**
    A feature's part of what an asset of a type becomes when it is dropped on the viewport. The
    model asset gets its model from graphics, and its collider from physics.
*/
struct EditorAssetDrop
{
    /// the URN type of the assets it takes, like 'mdl'
    Util::String type;
    Util::Delegate<void(EditorDroppedAsset& entity)> contribute;
};

//------------------------------------------------------------------------------
/**
    What the features have given the editor. A feature unit registers its parts here.

    A panel is the feature's own class, taken from EditorPanel, and the core draws it once per frame
    like any other panel, so the list is only pointers. Nothing here says which feature registered
    it, because nothing reads that. The core calls everything in the list.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
class EditorFeatureRegistry
{
public:
    static void RegisterPanel(Ptr<EditorPanel> panel);
    static void RegisterInspectorSection(Util::Delegate<void(Game::Entity)> draw);
    static void RegisterInspectorAddEntries(Util::Delegate<void(Game::Entity, Util::Array<InspectorEntry>&)> collect);
    static void RegisterAddComponentConstraint(const Util::Array<Game::ComponentId>& components, IndexT maxPerEntity);
    static void RegisterRuntimeComponents(const Util::Array<Game::ComponentId>& components);
    static void RegisterOnComponentChangeListener(Util::Delegate<void(Scene*, Game::Entity, Game::ComponentId)> onChange);
    static void RegisterStatusNumbers(Util::Delegate<void(Util::String& out)> append);
    /// A popup of the feature's own, like a picker. The core calls it every frame after the panels,
    /// with its own modals, and the popup opens and draws itself.
    static void RegisterModal(Util::Delegate<void()> draw);
    /// Work the feature holds outside the scenes, which the exit asks about with them.
    static void RegisterUnsavedWork(Util::Delegate<void(Util::Array<Util::String>&)> list, Util::Delegate<bool()> save);
    /// Give the entity an asset of the type becomes, when it is dropped on the viewport, its part.
    static void RegisterAssetDrop(const Util::String& type, Util::Delegate<void(EditorDroppedAsset&)> contribute);
    /// Give a feature the viewport's input each frame, after the gizmo handles and before the tools.
    /// It returns true to take the frame from the tools, which is how a mode that places points by
    /// clicking keeps the click from selecting. It can draw on the canvas as well.
    static void RegisterViewportInput(Util::Delegate<bool(const ToolView&)> input);
    /// Hand a frame to the features. True when one of them took it
    static bool OfferViewportInput(const ToolView& view);
    /// Add settings to the viewport's View popup, how the render side draws the canvas. The draw is
    /// given the view mode the canvas shows: scene, game, lighting or rendering
    static void RegisterViewportSettings(Util::Delegate<void(const char*)> draw);
    /// Draw them, inside the popup
    static void DrawViewportSettings(const char* viewMode);

    static const Util::Array<Ptr<EditorPanel>>& GetPanels();
    static const Util::Array<EditorInspectorSection>& GetSections();
    static const Util::Array<EditorInspectorAddEntrySource>& GetEntrySources();
    static const Util::Array<EditorAddComponentConstraint>& GetConstraints();
    static const Util::Array<EditorOnComponentChangeListener>& GetListeners();
    static const Util::Array<EditorStatusNumbers>& GetStatusNumbers();
    static const Util::Array<EditorModal>& GetModals();
    static const Util::Array<EditorUnsavedWork>& GetUnsavedWork();
    static const Util::Array<EditorAssetDrop>& GetAssetDrops();
    /// What the features make of a dropped asset. False when no feature takes its type, or none gave
    /// the entity anything, so the asset can't be dropped.
    static bool BuildAssetDrop(const IO::Path& asset, EditorDroppedAsset& out);

    /// Return true when an entity cannot be given this component, because a registered number is
    /// already full.
    static bool IsBlocked(Scene* scene, Game::Entity entity, Game::ComponentId component);

    /// Return true when the component is runtime state a feature named. A panel that reads the
    /// registry does not draw a section for one, and an entity cannot be given one.
    static bool IsRuntimeComponent(Game::ComponentId component);

    /// The core gives the shell at Open. A static part that nothing injected reads the shell here.
    static void SetShell(Shell* shell);
    static Shell* GetShell();

    /// The editor is going away.
    static void Clear();

private:
    static Util::Array<Ptr<EditorPanel>> panels;
    static Util::Array<EditorInspectorSection> sections;
    static Util::Array<EditorInspectorAddEntrySource> entrySources;
    static Util::Array<EditorAddComponentConstraint> constraints;
    static Util::Array<Game::ComponentId> runtimeComponents;
    static Util::Array<EditorOnComponentChangeListener> listeners;
    static Util::Array<EditorStatusNumbers> statusNumbers;
    static Util::Array<EditorModal> modals;
    static Util::Array<EditorUnsavedWork> unsavedWork;
    static Util::Array<EditorAssetDrop> assetDrops;
    static Util::Array<Util::Delegate<bool(const ToolView&)>> viewportInputs;
    static Util::Array<Util::Delegate<void(const char*)>> viewportSettings;
    static Shell* shell;
};

} // namespace Editor
//------------------------------------------------------------------------------
