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
    DefineInt64(FolderId, 'FLDI', Attr::ReadOnly);
    DefineInt64(ParentFolderId, 'PFID', Attr::ReadWrite);
    DefineBool(IsRootFolder, 'IROT', Attr::ReadOnly);
    DefineBool(IsArchive, 'IARC', Attr::ReadOnly);

    
    // File table attributes
    DefineInt64(FileId, 'FIDI', Attr::ReadOnly);
    DefineInt64(FileFolderId, 'FFID', Attr::ReadWrite);
    DefineInt64(FileSize, 'FSIZ', Attr::ReadWrite);
    DefineInt(FileType, 'FTYP', Attr::ReadWrite);
    DefineString(FileExt, 'FEXT', Attr::ReadWrite);
    
    // shared attributes (used by both folders and files)
    DefineString(EntityUri, 'FURI', Attr::ReadWrite);
    DefineInt64(ModifiedDate, 'MDAT', Attr::ReadWrite);
    DefineString(EntityName, 'NAME', Attr::ReadWrite);  // Generic name for folders/files    
}
