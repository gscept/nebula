#pragma once
//------------------------------------------------------------------------------
/**
	@file nsbindings.h

	Bindings specifically for C# scripting

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "nsharp/nsconfig.h"

namespace Scripting
{

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Print(const char* string, int32_t isStdout);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Error(const char* string, int32_t isStdout);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Log(const char* logDomain, const char* logLevel, const char* message, int32_t fatal, void* userData);

//------------------------------------------------------------------------------
/**
*/
NEBULA_EXPORT void N_Assert(bool value);

//------------------------------------------------------------------------------
/**
    The managed write path calls this when it writes a field. It reports the text the entity had and the text
    it would have after the write, and the editor turns that into a command. Nothing is attached in a build
    with no editor, and this does nothing for that build.

    The path is the key inside the text the change is about, as the extension names it. A path with no field
    is the whole property, which is what an add and a removal report.

    This is here rather than in propertyextension.cc, because the object library is what puts the symbol in
    the executable's dynamic symbol table. A symbol from the static nsharp archive is not one a managed
    DllImport can find.
*/
NEBULA_EXPORT void RecordPropertyChange(uint64_t entityId, const char* path, const char* oldText, const char* newText);

//------------------------------------------------------------------------------
/**
    The managed inspector calls this when the override marker on a row is pressed. It says which key was
    pressed on, and the editor decides what that means: the value the prefab chain gives for that key, put
    back over what the instance authored for the rest. Nothing is attached in a build with no editor, and this
    does nothing for that build.

    Same reason as RecordPropertyChange: this has to be in the object library for a managed DllImport to find
    it.
*/
NEBULA_EXPORT void RequestPropertyRevert(uint64_t entityId, const char* path);

//------------------------------------------------------------------------------
/**
    The managed inspector calls this for a row, to ask whether the editor's marker says the entity authored
    that key. The marker is the editor's own and the managed side cannot read it, so this is the only way a row
    learns whether it is the instance's own. A path with no field asks whether any field of the property is
    authored, which is what a section row asks with.

    Same reason as RecordPropertyChange: this has to be in the object library for a managed DllImport to find
    it.
*/
NEBULA_EXPORT bool PropertyExtension_IsAuthored(uint64_t entityId, const char* path);

//------------------------------------------------------------------------------
/**
    The property names the editor's dropdown lists. The managed side mirrors its registry into the extension
    with these, because the panel reads names and text and has no way to read a managed dictionary.

    Same reason as RecordPropertyChange: these have to be in the object library for a managed DllImport to
    find them.
*/
NEBULA_EXPORT void PropertyExtension_AddName(const char* propertyName);
NEBULA_EXPORT void PropertyExtension_ClearNames();

} // namespace Scripting
