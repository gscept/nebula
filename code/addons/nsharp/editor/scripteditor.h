#pragma once
//------------------------------------------------------------------------------
/**
    The scripting feature's editor contribution.

    The editor cannot read what a managed property holds, so this side gives it the inspector section,
    the add dropdown entries, and the text a new property starts with. The recorder is generic and
    resolves the extension by name.

    @see Editor::EditorFeatureRegistry
    @see Scripting::PropertyLevelExtension

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR

namespace Scripting
{

/// register the section, the entry source and the recorder callbacks
void RegisterEditorContributions();

/// take the recorder callbacks back
void ClearEditorContributions();

} // namespace Scripting

#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
