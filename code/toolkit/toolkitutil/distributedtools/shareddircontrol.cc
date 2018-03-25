//------------------------------------------------------------------------------
//  shareddircontrol.cc
//  (C) 2009 RadonLabs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "shareddircontrol.h"

using namespace IO;
using namespace Util;

namespace DistributedTools
{
    __ImplementClass(DistributedTools::SharedDirControl,'DSDC',Core::RefCounted)
//------------------------------------------------------------------------------
/**
    Constructor	
*/
SharedDirControl::SharedDirControl()
{
}

//------------------------------------------------------------------------------
/**
    Destructor	
*/
SharedDirControl::~SharedDirControl()
{
}

//------------------------------------------------------------------------------
/**
    set up shared directory (creates subdir from guid)
    and init private 'sharedDir' member
*/
void
SharedDirControl::SetUp()
{
    this->sharedDir = this->path;
    this->sharedDir.AppendLocalPath(this->guid.AsString());
    this->CreateControlDir();
}

//------------------------------------------------------------------------------
/**
   clean up shared directory (remove everything inside)
*/
void
SharedDirControl::CleanUp()
{
    this->RemoveDirectoryContent(this->sharedDir.AsString());
    this->RemoveControlDir();
}

//------------------------------------------------------------------------------
/**
    remove all files and directories of specified
    directory (relative to control dir) recursively 
*/
void
SharedDirControl::RemoveDirectoryContent(const Util::String & dir)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CopyFileToSharedDir(const Util::String & src, const Util::String & dst)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CopyFilesToSharedDir(const Util::String & src, const Util::String & dst)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CopyDirectoryContentToSharedDir(const Util::String & src)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CopyFileFromSharedDir(const Util::String & src, const Util::String & dst)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CopyFilesFromSharedDir(const Util::String & src, const Util::String & dst)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CopyDirectoryContentFromSharedDir(const Util::String & dst)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::RemoveFileInSharedDir(const Util::String & filepath)
{
    // empty, override in subclass
}
    
//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::RemoveDirectoryInSharedDir(const Util::String & dirpath)
{
    // empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
bool
SharedDirControl::ContainsGuidSubDir(const Util::Guid & guid)
{
    // override in subclass
    return false;
}


//------------------------------------------------------------------------------
/**
*/
bool
SharedDirControl::DirIsEmpty(const Util::String & dir)
{
    // override in subclass
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
SharedDirControl::ListDirectories(const Util::String & dir)
{
    // override in subclass
    Array<String> res;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String>
SharedDirControl::ListFiles(const Util::String & dir)
{
    // override in subclass
    Array<String> res;
    return res;
}
 
//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::CreateControlDir()
{
    // empty, override in subclass
}
    
//------------------------------------------------------------------------------
/**
*/
void
SharedDirControl::RemoveControlDir()
{
    // empty, override in subclass
}

} // namespace DistributedTools
