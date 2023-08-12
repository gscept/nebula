//------------------------------------------------------------------------------
//  archive.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/archfs/archive.h"

namespace IO
{
#if __WIN32__ || __linux__
__ImplementClass(IO::Archive, 'ARCV', IO::ZipArchive);
#else
#error "IO::Archive not implemented on this platform!"
#endif

} // namespace IO
