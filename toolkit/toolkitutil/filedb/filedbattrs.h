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
    DeclareGuid(FolderId, 'FLDI', Attr::ReadOnly);
    DeclareGuid(ParentFolderId, 'PFID', Attr::ReadWrite);
    DeclareBool(IsRootFolder, 'IROT', Attr::ReadOnly);
    DeclareBool(IsArchive, 'IARC', Attr::ReadOnly);
    
    // File table attributes
    DeclareGuid(FileId, 'FIDI', Attr::ReadOnly);
    DeclareGuid(FileFolderId, 'FFID', Attr::ReadWrite);
    DeclareInt64(FileSize, 'FSIZ', Attr::ReadWrite);
    DeclareInt(FileType, 'FTYP', Attr::ReadWrite);
    DeclareString(FileExt, 'FEXT', Attr::ReadWrite);
    
    // shared attributes (used by both folders and files)
    DeclareString(EntityUri, 'FURI', Attr::ReadWrite);
    DeclareUInt64(ModifiedDate, 'MDAT', Attr::ReadWrite);
    DeclareString(EntityName, 'NAME', Attr::ReadWrite);  // Generic name for folders/files    
}
