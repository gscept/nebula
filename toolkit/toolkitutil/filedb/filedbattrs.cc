//------------------------------------------------------------------------------
//  filedbattrs.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "filedbattrs.h"


//------------------------------------------------------------------------------
namespace Attr
{
    // Folder table attributes
    DefineAttrInt64(FolderId, 'FLDI', Attr::ReadOnly);
    DefineAttrInt64(ParentFolderId, 'PFID', Attr::ReadWrite);
    DefineAttrBool(IsRootFolder, 'IROT', Attr::ReadOnly);
    DefineAttrBool(IsArchive, 'IARC', Attr::ReadOnly);

    
    // File table attributes
    DefineAttrInt64(FileId, 'FIDI', Attr::ReadOnly);
    DefineAttrInt64(FileFolderId, 'FFID', Attr::ReadWrite);
    DefineAttrInt64(FileSize, 'FSIZ', Attr::ReadWrite);
    DefineAttrInt(FileType, 'FTYP', Attr::ReadWrite);
    DefineAttrString(FileExt, 'FEXT', Attr::ReadWrite);
    
    // shared attributes (used by both folders and files)
    DefineAttrString(EntityUri, 'FURI', Attr::ReadWrite);
    DefineAttrInt64(ModifiedDate, 'MDAT', Attr::ReadWrite);
    DefineAttrString(EntityName, 'NAME', Attr::ReadWrite);  // Generic name for folders/files    
}
