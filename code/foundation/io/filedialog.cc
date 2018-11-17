//------------------------------------------------------------------------------
//  filedialog.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/filedialog.h"
#include "nfd.h"


namespace IO
{

//------------------------------------------------------------------------------
/**
*/
bool 
FileDialog::OpenFile(const IO::URI & start, const Util::String& filters, Util::String& path)
{
    Util::String localpath = start.LocalPath();

#ifdef __WIN32__
    localpath.SubstituteChar('/', '\\');
#endif
    nfdchar_t * nfdpath = NULL;
    if (NFD_OKAY == NFD_OpenDialog(filters.AsCharPtr(), localpath.AsCharPtr(), &nfdpath))
    {
        path = nfdpath;
        free(nfdpath);
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
/**
*/
bool 
FileDialog::OpenFolder(const IO::URI & start, Util::String& path)
{
    Util::String localpath = start.LocalPath();

#ifdef __WIN32__
    localpath.SubstituteChar('/', '\\');
#endif
    nfdchar_t * nfdpath = NULL;
    if (NFD_OKAY == NFD_PickFolder(localpath.AsCharPtr(), &nfdpath))
    {
        path = nfdpath;
        free(nfdpath);
        return true;
    }
    return false;
}
    
//------------------------------------------------------------------------------
/**
*/
bool 
FileDialog::SaveFile(const IO::URI & start, const Util::String& filters, Util::String& path)
{
    Util::String localpath = start.LocalPath();

#ifdef __WIN32__
    localpath.SubstituteChar('/', '\\');
#endif
    nfdchar_t * nfdpath = NULL;
    if (NFD_OKAY == NFD_SaveDialog(filters.AsCharPtr(), localpath.AsCharPtr(), &nfdpath))
    {
        path = nfdpath;
        free(nfdpath);
        return true;
    }
    return false;
}
}