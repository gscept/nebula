#pragma once
//------------------------------------------------------------------------------
/**
    @file core/osx/precompiled.h
 
    Contains precompiled headers on the OSX platform.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
*/

// crt headers
// GNU/C runtime
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <malloc/malloc.h>
#include <uuid/uuid.h>

// C++ runtime
#include <algorithm>

// NOTE: no headers here which depend on ObjC!