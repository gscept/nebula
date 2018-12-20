//------------------------------------------------------------------------------
//  filedialog.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/filedialog.h"
#include "tinyfiledialogs.h"
#include "ioserver.h"
#include <typeinfo>
namespace IO
{
namespace FileDialog
{
//------------------------------------------------------------------------------
/**
*/
bool 
OpenFile(Util::String const& title, const IO::URI & start, std::initializer_list<const char*>const& filters, Util::String& path)
{
    Util::String localpath = start.LocalPath();
    if (!IO::IoServer::Instance()->FileExists(start))
    {
        localpath.Append("/");
    }
#ifdef __WIN32__
    localpath.SubstituteChar('/', '\\');
#endif
    
    char const * filterlist[16];
    n_assert2(filters.size() < 16, "Too large initializer list");
    int c = 0;
    for (auto i : filters)
    {
        filterlist[c++] = i;
    }
    
    const char * fpath = tinyfd_openFileDialog(title.AsCharPtr(), localpath.AsCharPtr(), c, filterlist, NULL, false);
    if (fpath)
    {
        path = fpath;
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
/**
*/
bool 
OpenFolder(Util::String const& title, const IO::URI & start, Util::String& path)
{
    Util::String localpath = start.LocalPath();
#ifdef __WIN32__
    localpath.SubstituteChar('/', '\\');
#endif
    const char * fpath = tinyfd_selectFolderDialog(title.AsCharPtr(), localpath.AsCharPtr());
    if (fpath)
    {
        path = fpath;
        return true;
    }
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
bool 
SaveFile(Util::String const& title, const IO::URI & start, std::initializer_list<const char*>const& filters, Util::String& path)
{
    Util::String localpath = start.LocalPath();
    if (!IO::IoServer::Instance()->FileExists(start))
    {
        localpath.Append("/");
    }
#ifdef __WIN32__
    localpath.SubstituteChar('/', '\\');
#endif

    char const * filterlist[16];
    n_assert2(filters.size() < 16, "Too large initializer list");
    int c = 0;
    for (auto i : filters)
    {
        filterlist[c++] = i;
    }

    const char * fpath = tinyfd_saveFileDialog(title.AsCharPtr(), localpath.AsCharPtr(), c, filterlist, NULL);
    if (fpath)
    {
        path = fpath;
        return true;
    }
    return false;
}

} // namespace FileDialog
} // namespace IO