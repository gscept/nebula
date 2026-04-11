#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FileDbAttrs
    
    Attribute declarations for the FileDB hierarchical file metadata database.
    
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "attr/attribute.h"

//------------------------------------------------------------------------------
namespace Attr
{
    // Folder table attributes
    DeclareInt64(FolderId, 'FLDI', Attr::ReadOnly);
    DeclareInt64(ParentFolderId, 'PFID', Attr::ReadWrite);
    DeclareBool(IsRootFolder, 'IROT', Attr::ReadOnly);
    DeclareBool(IsArchive, 'IARC', Attr::ReadOnly);
    
    // File table attributes
    DeclareInt64(FileId, 'FIDI', Attr::ReadOnly);
    DeclareInt64(FileFolderId, 'FFID', Attr::ReadWrite);
    DeclareInt64(FileSize, 'FSIZ', Attr::ReadWrite);
    DeclareInt(FileType, 'FTYP', Attr::ReadWrite);
    DeclareString(FileExt, 'FEXT', Attr::ReadWrite);
    
    // shared attributes (used by both folders and files)
    DeclareString(EntityUri, 'FURI', Attr::ReadWrite);
    DeclareInt64(ModifiedDate, 'MDAT', Attr::ReadWrite);
    DeclareString(EntityName, 'NAME', Attr::ReadWrite);  // Generic name for folders/files    
}
