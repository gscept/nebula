//------------------------------------------------------------------------------
//  path.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/path.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "core/config.h"

//------------------------------------------------------------------------------
/**
    Literal constructor form string, to use "foobar"_urn will automatically construct an IO::URN
*/
IO::Path
operator""_path(const char* c, std::size_t s)
{
    return IO::Path::Parse(c);
}

namespace IO
{

Util::Dictionary<Util::StringAtom, Util::StringAtom> Path::ExportExtensions, Path::WorkExtensions;
Util::StringAtom Path::WorkRoot;

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
Path::Parse(const Util::String& serialized)
{
    Path ret;

    IndexT folderSeparatorIndex = serialized.FindCharIndex(':');
    IndexT fileSeparatorIndex = serialized.FindCharIndex(':', folderSeparatorIndex + 1);
    n_assert_fmt(folderSeparatorIndex != InvalidIndex && fileSeparatorIndex != InvalidIndex, "String %s must be a valid path (folder:<file>:<type>", serialized.AsCharPtr());

    ret.folder = serialized.ExtractRange(0, folderSeparatorIndex);
    ret.file = serialized.ExtractRange(folderSeparatorIndex + 1, fileSeparatorIndex - folderSeparatorIndex - 1);
    ret.type = serialized.ExtractToEnd(fileSeparatorIndex + 1);
    ret.Build();
   
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::Folder(const Util::String& folder)
{
    Path ret;
    ret.folder = folder;
    ret.folder.TrimRight("/\\");
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
    ret.folder.TrimRight("/\\");
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
Path::FolderAndFile(const char* type, const Util::String& folderAndFile)
{
    Path ret;
    ret.folder = folderAndFile.ExtractToLastSlash();
    ret.folder.TrimRight("/\\");
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
    ret.folder.TrimRight("/\\");
    ret.file = file;
    ret.type = type;
    ret.isFile = true;
    ret.Build();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::File(const Util::String& folder, const Util::String& file, const char* type)
{
    Path ret;
    ret.folder = folder;
    ret.folder.TrimRight("/\\");
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
const Util::String
Path::AsString() const
{
    return Util::Format("%s:%s:%s", this->folder.AsCharPtr(), this->file.AsCharPtr(), this->type.AsCharPtr());
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
bool
Path::operator<(const Path& path) const
{
    return this->folderAndFile < path.folderAndFile;
}

//------------------------------------------------------------------------------
/**
*/
bool
Path::operator>(const Path& path) const
{
    return this->folderAndFile > path.folderAndFile;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::operator/(const char* folder) const
{
    Path ret = *this;
    if (!ret.IsEmpty())
        ret.folder += "/" + Util::String(folder);
    else
        ret.folder += Util::String(folder);
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
    if (!ret.IsEmpty())
        ret.folder += "/" + folder;
    else
        ret.folder += folder;
    ret.Build();
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Path
Path::operator/(const IO::Path& folder) const
{
    Path ret = *this;
    if (!ret.IsEmpty())
        ret.folder += "/" + folder.folder;
    else
        ret.folder += folder.folder;
    ret.Build();
    return ret;
}


//------------------------------------------------------------------------------
/**
*/
URI
Path::WorkURI(const char* workFolder) const
{
    n_assert(Path::WorkRoot.IsValid());
    if (!this->file.IsValid())
    {
        return IO::URI(Util::Format("%s/%s", Path::WorkRoot.Value(), (Util::String(workFolder) + "/" + this->folderAndFile).AsCharPtr()));
    }
    else
    {
        IndexT i = Path::WorkExtensions.FindIndex(this->type);
        if (i == InvalidIndex)
        {
            return IO::URI(Util::Format("%s/%s", Path::WorkRoot.Value(), (Util::String(workFolder) + "/" + this->folderAndFile).AsCharPtr()));
        }
        else
        {
            return IO::URI(Util::Format("%s/%s.%s", Path::WorkRoot.Value(), (Util::String(workFolder) + "/" + this->folderAndFile).AsCharPtr(), Path::WorkExtensions.ValueAtIndex(i).Value()));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
URI
Path::WorkURI(const Util::String& workFolder) const
{
    n_assert(Path::WorkRoot.IsValid());
    if (!this->file.IsValid())
    {
        return IO::URI(Util::Format("%s/%s", Path::WorkRoot.Value(), (workFolder + "/" + this->folderAndFile).AsCharPtr()));
    }
    else
    {
        IndexT i = Path::WorkExtensions.FindIndex(this->type);
        if (i == InvalidIndex)
        {
            return IO::URI(Util::Format("%s/%s", Path::WorkRoot.Value(), (workFolder + "/" + this->folderAndFile).AsCharPtr()));
        }
        else
        {
            return IO::URI(Util::Format("%s/%s.%s", Path::WorkRoot.Value(), (workFolder + "/" + this->folderAndFile).AsCharPtr(), Path::WorkExtensions.ValueAtIndex(i).Value()));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
URI
Path::ExportURI() const
{
    IndexT i = Path::ExportExtensions.FindIndex(this->type);
    n_assert(i != InvalidIndex);
    return IO::URI(
        Util::Format("%s:%s.%s", this->type.AsCharPtr(), this->folderAndFile.AsCharPtr(), Path::ExportExtensions.ValueAtIndex(i).Value())
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
Path::SetFile(const Util::String& file, const Util::String& type)
{
    this->file = file;
    this->type = type;
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
Util::String
Path::GetWorkFile() const
{
    IndexT i = Path::WorkExtensions.FindIndex(this->type);
    if (i != InvalidIndex)
        return Util::Format("%s.%s", this->file.AsCharPtr(), Path::WorkExtensions.ValueAtIndex(i).Value());
    else
        return this->file;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
Path::GetExportFile() const
{
    IndexT i = Path::ExportExtensions.FindIndex(this->type);
    n_assert(i != InvalidIndex);
    return Util::Format("%s.%s", this->folderAndFile.AsCharPtr(), Path::ExportExtensions.ValueAtIndex(i).Value());
}

//------------------------------------------------------------------------------
/**
*/
void
Path::Build()
{
    if (!this->file.IsEmpty())
        this->folderAndFile = this->folder + "/" + this->file;
    else
        this->folderAndFile = this->folder;
}

} // namespace IO
