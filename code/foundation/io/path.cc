//------------------------------------------------------------------------------
//  path.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/path.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "core/config.h"

namespace IO
{

//------------------------------------------------------------------------------
/**
*/
Path::Path()
    : isFile(false)
{
}

//------------------------------------------------------------------------------
/**
*/
Path::~Path()
{
}

//------------------------------------------------------------------------------
/**
*/
Path::Path(const Path& path)
    : folder(path.folder)
    , file(path.file)
    , type(path.type),
      isFile(path.isFile)
{
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::Folder(const Util::String& folder)
{
    Path ret;
    ret.folder = folder;
    ret.Build();
    ret.isFile = false;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::FolderAndFile(const Util::String& type, const Util::String& folderAndFile)
{
    Path ret;
    ret.folder = folderAndFile.ExtractToLastSlash();
    ret.file = folderAndFile.ExtractFileName();
    ret.type = type;
    ret.isFile = true;
    ret.Build();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::File(const Util::String& folder, const Util::String& file, const Util::String& type)
{
    Path ret;
    ret.folder = folder;
    ret.file = file;
    ret.type = type;
    ret.isFile = true;
    ret.Build();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
Path::Clear()
{
    this->folder.Clear();
    this->file.Clear();
    this->type.Clear();
    this->isFile = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
Path::IsEmpty() const
{
    return this->folder.IsEmpty() && this->file.IsEmpty();
}

//------------------------------------------------------------------------------
/**
*/
bool
Path::IsFolder() const
{
    return !this->isFile;
}

//------------------------------------------------------------------------------
/**
*/
bool
Path::IsFile() const
{
    return this->isFile;
}

//------------------------------------------------------------------------------
/**
*/
bool
Path::operator==(const Path& path) const
{
    return this->folderAndFile == path.folderAndFile && this->type == path.type;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::operator/(const char* folder) const
{
    Path ret = *this;
    ret.folder += "/" + Util::String(folder);
    ret.Build();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::operator/(const Util::String& folder) const
{
    Path ret = *this;
    ret.folder += "/" + Util::String(folder);
    ret.Build();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
URI
Path::WorkURI(const char* workFolder) const
{
    n_assert(URN::WorkRoot.IsValid());
    n_assert(this->file.IsValid())
    IndexT i = URN::WorkExtensions.FindIndex(this->type);
    if (i == InvalidIndex)
    {
        return IO::URI(Util::Format("%s/%s", URN::WorkRoot.Value(), (Util::String(workFolder) + "/" + this->folderAndFile).AsCharPtr()));
    }
    else
    {
        return IO::URI(Util::Format("%s/%s.%s", URN::WorkRoot.Value(), (Util::String(workFolder) + "/" + this->folderAndFile).AsCharPtr(), URN::WorkExtensions.ValueAtIndex(i).Value()));
    }
}

//------------------------------------------------------------------------------
/**
*/
URI
Path::ExportURI() const
{
    IndexT i = URN::ExportExtensions.FindIndex(this->type);
    n_assert(i != InvalidIndex);
    return IO::URI(
        Util::Format("%s:%s.%s", this->type.AsCharPtr(), this->folderAndFile.AsCharPtr(), URN::ExportExtensions.ValueAtIndex(i).Value())
    );
}

//------------------------------------------------------------------------------
/**
*/
void
Path::SetFolder(const Util::String& folder)
{
    this->folder = folder;
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
void
Path::ClearFolder()
{
    this->folder.Clear();
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
void
Path::SetFile(const Util::String& file)
{
    this->file = file;
    this->isFile = true;
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
void
Path::ClearFile()
{
    this->file.Clear();
    this->isFile = false;
    this->Build();
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
Path::GetFolderAndFile() const
{
    return this->folderAndFile;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
Path::GetFile() const
{
    return this->file;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
Path::GetFolder() const
{
    return this->folder;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
Path::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
void
Path::Build()
{
    this->folderAndFile = this->folder + "/" + this->file;
}

} // namespace IO
