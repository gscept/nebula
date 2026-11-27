#pragma once
//------------------------------------------------------------------------------
/**
    @file resourcefield.h
    The inspector's resource fields drawn as asset fields. A component field that holds a resource,
    a field of type resource or a string that names a resourceType in its NIDL file, is shown by the folder
    and file of the asset it refers to, and takes the assets of its type from the browser, the
    clipboard and the asset picker. A field that names no type is edited as text, since it may hold
    a path its loader reads rather than a URN.

    The inspector edits a copy of the component and makes the change from what the copy holds after
    the draw. A drop or a paste is made while the field draws, so it goes into the copy then. The
    picker answers after the frame, so its answer is kept and written the next time the same field
    of the same entity draws, which is the frame after. An answer the field is not drawn for, because
    the inspector shows something else by then, is dropped.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Editor
{

/// draw the inspector's resource fields as asset fields
void SetupResourceFields();
/// draw them as the inspection system does without the editor
void DiscardResourceFields();

} // namespace Editor
//------------------------------------------------------------------------------
