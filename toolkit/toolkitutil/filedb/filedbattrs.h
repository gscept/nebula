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
    DeclareAttrInt64(FolderId);
    DeclareAttrInt64(ParentFolderId);
    DeclareAttrBool(IsRootFolder);
    DeclareAttrBool(IsArchive);
    
    // File table attributes
    DeclareAttrInt64(FileId);
    DeclareAttrInt64(FileFolderId);
    DeclareAttrInt64(FileSize);
    DeclareAttrInt(FileType);
    DeclareAttrString(FileExt);
    
    // shared attributes (used by both folders and files)
    DeclareAttrString(EntityUri);
    DeclareAttrInt64(ModifiedDate);
    DeclareAttrString(EntityName);  // Generic name for folders/files    
}
