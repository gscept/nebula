#pragma once
/* ionebula3.h -- IO base function header for compress/uncompress .zip
   files using zlib + zip or unzip API
   This IO API version uses the Nebula3 file API.

   (C) 2007 Radon Labs GmbH
   (C) 2020 Individual contributors, see AUTHORS file
*/

#include "minizip/ioapi.h"

void fill_nebula3_filefunc(zlib_filefunc64_def* pzlib_filefunc_def);
