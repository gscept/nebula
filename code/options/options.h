#pragma once
//------------------------------------------------------------------------------
/**
    Globally accessible options
    
    (C) 2025 Individual contributors, see AUTHORS file 
*/
//------------------------------------------------------------------------------
#include "nflatbuffer/nebula_flat.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "flat/options/levelsettings.h"
#include "flat/options/projectsettings.h"

namespace Options 
{

extern App::ProjectSettingsT ProjectSettings;
extern App::LevelSettingsT LevelSettings;

//------------------------------------------------------------------------------
/**
*/
extern void InitOptions();

} // namespace Options
